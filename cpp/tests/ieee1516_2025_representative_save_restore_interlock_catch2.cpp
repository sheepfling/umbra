#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded federation save and restore interlock representative services",
    "[integration][development-profile][federation-management][save-restore][interlocks]"
    "[rti.service.publish-object-class-attributes][rti.service.register-object-instance]"
    "[rti.service.create-region][rti.service.update-attribute-values]"
    "[rti.service.send-interaction][rti.service.attribute-ownership-acquisition]"
    "[rti.service.time-advance-request]"
    "[rti.service.object-class-declaration-interlocks]"
    "[rti.service.interaction-declaration-interlocks]"
    "[rti.service.directed-declaration-interlocks]"
    "[rti.service.order-type-interlocks]"
    "[rti.service.transportation-type-interlocks]"
    "[rti.service.ownership-disposition-interlocks]"
    "[rti.service.time-role-query-interlocks]"
    "[rti.service.scope-advisory-interlocks]"
    "[rti.service.regional-service-interlocks]"
    "[rti.service.synchronization-interlocks]"
    "[representative-save-restore-interlock-test]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  VariableLengthData tag;

  REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(peer->connect(peerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"interlock-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(L"interlock-peer", L"peer", federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  auto const takeOrder = owner->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  auto const reliable = owner->getTransportationTypeHandle(standard_hla::mom::reliable);
  auto const ownerHandle = owner->getFederateHandle(L"interlock-owner");
  AttributeHandleSet const efficiencyOnly{efficiency};
  InteractionClassHandleSet const takeOrderOnly{takeOrder};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(owner->publishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->subscribeInteractionClass(takeOrder));
  REQUIRE_NOTHROW(owner->publishObjectClassDirectedInteractions(server, takeOrderOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassDirectedInteractions(server, takeOrderOnly));
  auto objectInstance = owner->registerObjectInstance(server);
  AttributeHandleSet divestedAttributes;
  rti1516_2025::HLAinteger64Time interlockTime;
  rti1516_2025::HLAinteger64Interval interlockLookahead(1);
  auto const interlockRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      interlockRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{interlockRegion}));
  AttributeHandleSetRegionHandleSetPairVector const interlockAttributeRegions{{
      efficiencyOnly,
      RegionHandleSet{interlockRegion},
  }};
  RegionHandleSet const interlockRegions{interlockRegion};

  REQUIRE_NOTHROW(owner->requestFederationSave(L"interlock-save"));
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveBegun());

  REQUIRE_THROWS_AS(
      owner->publishObjectClassAttributes(server, efficiencyOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishObjectClass(server),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishObjectClassAttributes(server, efficiencyOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      peer->subscribeObjectClassAttributes(server, efficiencyOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeObjectClass(server),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeObjectClassAttributes(server, efficiencyOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->publishInteractionClass(takeOrder),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishInteractionClass(takeOrder),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      peer->subscribeInteractionClass(takeOrder),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeInteractionClass(takeOrder),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->publishObjectClassDirectedInteractions(server, takeOrderOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishObjectClassDirectedInteractions(server),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishObjectClassDirectedInteractions(server, takeOrderOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      peer->subscribeObjectClassDirectedInteractions(server, takeOrderOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeObjectClassDirectedInteractions(server),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeObjectClassDirectedInteractions(server, takeOrderOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->changeAttributeOrderType(objectInstance, efficiencyOnly, RECEIVE),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->changeDefaultAttributeOrderType(server, efficiencyOnly, RECEIVE),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->changeInteractionOrderType(takeOrder, RECEIVE),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->requestAttributeTransportationTypeChange(objectInstance, efficiencyOnly, reliable),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->changeDefaultAttributeTransportationType(server, efficiencyOnly, reliable),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->queryAttributeTransportationType(objectInstance, efficiency),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->requestInteractionTransportationTypeChange(takeOrder, reliable),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->queryInteractionTransportationType(ownerHandle, takeOrder),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->queryAttributeOwnership(objectInstance, efficiencyOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->isAttributeOwnedByFederate(objectInstance, efficiency),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->negotiatedAttributeOwnershipDivestiture(objectInstance, efficiencyOnly, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(objectInstance, efficiencyOnly, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->cancelNegotiatedAttributeOwnershipDivestiture(objectInstance, efficiencyOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(objectInstance, efficiencyOnly, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipReleaseDenied(objectInstance, efficiencyOnly, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipDivestitureIfWanted(
          objectInstance, efficiencyOnly, tag, divestedAttributes),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->cancelAttributeOwnershipAcquisition(objectInstance, efficiencyOnly),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->registerObjectInstance(server),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->createRegion(DimensionHandleSet{sodaFlavor}),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->updateAttributeValues(objectInstance, AttributeHandleValueMap{}, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->sendInteraction(takeOrder, ParameterHandleValueMap{}, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipAcquisition(objectInstance, efficiencyOnly, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->enableTimeRegulation(interlockLookahead),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->disableTimeRegulation(), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->enableTimeConstrained(), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->disableTimeConstrained(), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->queryLogicalTime(interlockTime), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->queryGALT(interlockTime), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->queryLITS(interlockTime), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->queryLookahead(interlockLookahead), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(owner->modifyLookahead(interlockLookahead), rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->getAttributeScopeAdvisorySwitch(),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->setAttributeScopeAdvisorySwitch(false),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->registerObjectInstanceWithRegions(server, interlockAttributeRegions),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->associateRegionsForUpdates(objectInstance, interlockAttributeRegions),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unassociateRegionsForUpdates(objectInstance, interlockAttributeRegions),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->subscribeObjectClassAttributesWithRegions(server, interlockAttributeRegions),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unsubscribeObjectClassAttributesWithRegions(server, interlockAttributeRegions),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->requestAttributeValueUpdateWithRegions(server, interlockAttributeRegions, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->subscribeInteractionClassWithRegions(takeOrder, interlockRegions),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->unsubscribeInteractionClassWithRegions(takeOrder, interlockRegions),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->sendInteractionWithRegions(
          takeOrder, ParameterHandleValueMap{}, interlockRegions, tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->registerFederationSynchronizationPoint(L"interlock-sync", tag),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->registerFederationSynchronizationPoint(
          L"interlock-sync-set", tag, FederateHandleSet{ownerHandle}),
      rti1516_2025::SaveInProgress);
  REQUIRE_THROWS_AS(
      owner->synchronizationPointAchieved(L"interlock-sync"),
      rti1516_2025::SaveInProgress);

  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(peer->federateSaveComplete());
  REQUIRE_NOTHROW(owner->requestFederationRestore(L"interlock-save"));

  REQUIRE_THROWS_AS(
      owner->publishObjectClassAttributes(server, efficiencyOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishObjectClass(server),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishObjectClassAttributes(server, efficiencyOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      peer->subscribeObjectClassAttributes(server, efficiencyOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeObjectClass(server),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeObjectClassAttributes(server, efficiencyOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->publishInteractionClass(takeOrder),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishInteractionClass(takeOrder),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      peer->subscribeInteractionClass(takeOrder),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeInteractionClass(takeOrder),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->publishObjectClassDirectedInteractions(server, takeOrderOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishObjectClassDirectedInteractions(server),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unpublishObjectClassDirectedInteractions(server, takeOrderOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      peer->subscribeObjectClassDirectedInteractions(server, takeOrderOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeObjectClassDirectedInteractions(server),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      peer->unsubscribeObjectClassDirectedInteractions(server, takeOrderOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->changeAttributeOrderType(objectInstance, efficiencyOnly, RECEIVE),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->changeDefaultAttributeOrderType(server, efficiencyOnly, RECEIVE),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->changeInteractionOrderType(takeOrder, RECEIVE),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->requestAttributeTransportationTypeChange(objectInstance, efficiencyOnly, reliable),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->changeDefaultAttributeTransportationType(server, efficiencyOnly, reliable),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->queryAttributeTransportationType(objectInstance, efficiency),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->requestInteractionTransportationTypeChange(takeOrder, reliable),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->queryInteractionTransportationType(ownerHandle, takeOrder),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->queryAttributeOwnership(objectInstance, efficiencyOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->isAttributeOwnedByFederate(objectInstance, efficiency),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->negotiatedAttributeOwnershipDivestiture(objectInstance, efficiencyOnly, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(objectInstance, efficiencyOnly, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->cancelNegotiatedAttributeOwnershipDivestiture(objectInstance, efficiencyOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(objectInstance, efficiencyOnly, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipReleaseDenied(objectInstance, efficiencyOnly, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipDivestitureIfWanted(
          objectInstance, efficiencyOnly, tag, divestedAttributes),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->cancelAttributeOwnershipAcquisition(objectInstance, efficiencyOnly),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->registerObjectInstance(server),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->createRegion(DimensionHandleSet{sodaFlavor}),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->updateAttributeValues(objectInstance, AttributeHandleValueMap{}, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->sendInteraction(takeOrder, ParameterHandleValueMap{}, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->attributeOwnershipAcquisition(objectInstance, efficiencyOnly, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->enableTimeRegulation(interlockLookahead),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(owner->disableTimeRegulation(), rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(owner->enableTimeConstrained(), rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(owner->disableTimeConstrained(), rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(owner->queryLogicalTime(interlockTime), rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(owner->queryGALT(interlockTime), rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(owner->queryLITS(interlockTime), rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(owner->queryLookahead(interlockLookahead), rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(owner->modifyLookahead(interlockLookahead), rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->getAttributeScopeAdvisorySwitch(),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->setAttributeScopeAdvisorySwitch(false),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->registerObjectInstanceWithRegions(server, interlockAttributeRegions),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->associateRegionsForUpdates(objectInstance, interlockAttributeRegions),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unassociateRegionsForUpdates(objectInstance, interlockAttributeRegions),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->subscribeObjectClassAttributesWithRegions(server, interlockAttributeRegions),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unsubscribeObjectClassAttributesWithRegions(server, interlockAttributeRegions),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->requestAttributeValueUpdateWithRegions(server, interlockAttributeRegions, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->subscribeInteractionClassWithRegions(takeOrder, interlockRegions),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->unsubscribeInteractionClassWithRegions(takeOrder, interlockRegions),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->sendInteractionWithRegions(
          takeOrder, ParameterHandleValueMap{}, interlockRegions, tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->registerFederationSynchronizationPoint(L"interlock-sync", tag),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->registerFederationSynchronizationPoint(
          L"interlock-sync-set", tag, FederateHandleSet{ownerHandle}),
      rti1516_2025::RestoreInProgress);
  REQUIRE_THROWS_AS(
      owner->synchronizationPointAchieved(L"interlock-sync"),
      rti1516_2025::RestoreInProgress);

  REQUIRE_NOTHROW(owner->federateRestoreComplete());
  REQUIRE_NOTHROW(peer->federateRestoreComplete());
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
} // namespace
