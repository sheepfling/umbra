#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded 2025 region templates preserve pending and committed range state",
    "[integration][development-profile][federation-management][ddm][region-lifecycle]"
    "[rti.service.create-region][rti.service.commit-region-modifications]"
    "[rti.service.delete-region][rti.service.get-dimension-handle-set]"
    "[rti.service.get-range-bounds][rti.service.set-range-bounds]"
    "[rti.service.decode-region-handle]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador foreignFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto foreign = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  RegionHandle invalidRegion;
  DimensionHandle invalidDimension;

  REQUIRE_THROWS_AS(
      unjoined->createRegion(DimensionHandleSet{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeRegionHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->createRegion(DimensionHandleSet{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeRegionHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getDimensionHandleSet(invalidRegion),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(foreign->connect(foreignFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"region-owner", L"owner", federationName));
  REQUIRE_NOTHROW(foreign->joinFederationExecution(L"region-foreign", L"foreign", federationName));

  // The official 2025 C++ createRegion surface accepts an empty set of
  // specified dimensions.  This is a valid zero-dimensional template and
  // specification; the sibling Python backend's InvalidRegionContext branch
  // is intentionally not copied because that exception is not declared by
  // the C++ API.
  auto const emptyRegion = owner->createRegion(DimensionHandleSet{});
  REQUIRE(emptyRegion.isValid());
  REQUIRE(owner->getDimensionHandleSet(emptyRegion).empty());
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{emptyRegion}));
  REQUIRE(owner->getDimensionHandleSet(emptyRegion).empty());
  REQUIRE_NOTHROW(owner->deleteRegion(emptyRegion));

  auto const barQuantity = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(barQuantity.isValid());
  REQUIRE(sodaFlavor.isValid());

  auto const region = owner->createRegion(DimensionHandleSet{barQuantity, sodaFlavor});
  REQUIRE(region.isValid());
  REQUIRE(owner->getDimensionHandleSet(region) == DimensionHandleSet{barQuantity, sodaFlavor});
  REQUIRE(owner->decodeRegionHandle(region.encode()) == region);
  REQUIRE_THROWS_AS(
      owner->decodeRegionHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);

  REQUIRE_THROWS_AS(
      owner->getRangeBounds(region, barQuantity),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->commitRegionModifications(RegionHandleSet{region}),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, invalidDimension, RangeBounds(0UL, 10UL)),
      rti1516_2025::RegionDoesNotContainSpecifiedDimension);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, barQuantity, RangeBounds(10UL, 10UL)),
      rti1516_2025::InvalidRangeBound);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, barQuantity, RangeBounds(0UL, 26UL)),
      rti1516_2025::InvalidRangeBound);

  REQUIRE_NOTHROW(owner->setRangeBounds(region, barQuantity, RangeBounds(0UL, 10UL)));
  auto const pendingBar = owner->getRangeBounds(region, barQuantity);
  REQUIRE(pendingBar.getLowerBound() == 0UL);
  REQUIRE(pendingBar.getUpperBound() == 10UL);
  REQUIRE_THROWS_AS(
      owner->commitRegionModifications(RegionHandleSet{region}),
      rti1516_2025::InvalidRegion);
  REQUIRE(owner->getRangeBounds(region, barQuantity).getUpperBound() == 10UL);
  REQUIRE_THROWS_AS(
      owner->getRangeBounds(region, sodaFlavor),
      rti1516_2025::InvalidRegion);

  REQUIRE_THROWS_AS(
      foreign->commitRegionModifications(RegionHandleSet{region}),
      rti1516_2025::RegionNotCreatedByThisFederate);
  REQUIRE_THROWS_AS(
      foreign->deleteRegion(region),
      rti1516_2025::RegionNotCreatedByThisFederate);
  REQUIRE_THROWS_AS(
      foreign->getDimensionHandleSet(region),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      foreign->getRangeBounds(region, barQuantity),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      foreign->setRangeBounds(region, barQuantity, RangeBounds(0UL, 5UL)),
      rti1516_2025::RegionNotCreatedByThisFederate);

  REQUIRE_NOTHROW(owner->setRangeBounds(region, sodaFlavor, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  auto const committedBar = owner->getRangeBounds(region, barQuantity);
  auto const committedSoda = owner->getRangeBounds(region, sodaFlavor);
  REQUIRE(committedBar.getLowerBound() == 0UL);
  REQUIRE(committedBar.getUpperBound() == 10UL);
  REQUIRE(committedSoda.getLowerBound() == 1UL);
  REQUIRE(committedSoda.getUpperBound() == 3UL);

  REQUIRE_NOTHROW(owner->setRangeBounds(region, barQuantity, RangeBounds(5UL, 15UL)));
  auto const replacement = owner->getRangeBounds(region, barQuantity);
  REQUIRE(replacement.getLowerBound() == 5UL);
  REQUIRE(replacement.getUpperBound() == 15UL);
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  auto const recommitted = owner->getRangeBounds(region, barQuantity);
  REQUIRE(recommitted.getLowerBound() == 5UL);
  REQUIRE(recommitted.getUpperBound() == 15UL);

  // A region remains in use for deletion purposes even when its regional
  // subscription is passive.  The declaration does not arrange delivery, but
  // the region is still a live subscription dependency until it is removed.
  auto const interactionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const serverId = owner->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(serverId.isValid());
  auto const subscriptionRegion = owner->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      subscriptionRegion,
      serverId,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(owner->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriptionRegion},
      false));
  REQUIRE_THROWS_AS(
      owner->deleteRegion(subscriptionRegion),
      rti1516_2025::RegionInUseForUpdateOrSubscription);
  REQUIRE_NOTHROW(owner->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(owner->deleteRegion(subscriptionRegion));

  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_THROWS_AS(
      owner->getDimensionHandleSet(region),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->deleteRegion(region),
      rti1516_2025::InvalidRegion);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(foreign->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(foreign->disconnect());
}

TEST_CASE(
    "Embedded unpublishing an associated regional attribute releases region use",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[region-lifecycle][publication]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.unpublish-object-class-attributes]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.delete-region]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"regional-publication-owner", L"owner", federationName));

  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = owner->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());

  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));

  auto const region = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      region,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      flavorOnly,
      RegionHandleSet{region},
  }};
  auto const objectInstance = owner->registerObjectInstanceWithRegions(
      soda,
      sourcePair);
  REQUIRE(objectInstance.isValid());

  // The registration's update-region association keeps the region in use.
  REQUIRE_THROWS_AS(
      owner->deleteRegion(region),
      rti1516_2025::RegionInUseForUpdateOrSubscription);

  // Unpublishing the attribute removes that association.  The region usage
  // ledger must be refreshed before the service returns so the same region
  // can be deleted immediately, without requiring an unrelated mutation.
  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_THROWS_AS(
      owner->getDimensionHandleSet(region),
      rti1516_2025::InvalidRegion);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded receive-order deletion releases a sole object's update region",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[region-lifecycle][object-deletion]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.delete-object-instance]"
    "[rti.service.delete-region]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"regional-deleting-owner", L"owner", federationName));

  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = owner->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());

  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));

  auto const region = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      region,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      flavorOnly,
      RegionHandleSet{region},
  }};
  auto const objectInstance = owner->registerObjectInstanceWithRegions(
      soda,
      sourcePair);
  REQUIRE(objectInstance.isValid());

  // The registered object's explicit update-region association protects the
  // region while the object is alive.
  REQUIRE_THROWS_AS(
      owner->deleteRegion(region),
      rti1516_2025::RegionInUseForUpdateOrSubscription);

  // With no surviving recipient, receive-order deletion purges the object in
  // the same service call.  The region ledger must be refreshed before return
  // so the former object's last association no longer blocks deletion.
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, VariableLengthData{}));
  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_THROWS_AS(
      owner->getDimensionHandleSet(region),
      rti1516_2025::InvalidRegion);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded final object-removal callback releases the deleted object's update region",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[region-lifecycle][object-deletion][callback-ordering]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.delete-object-instance]"
    "[rti.service.delete-region]"
    "[federate.callback.remove-object-instance]") {
  TestFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"regional-removal-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"regional-removal-peer", L"subscriber", federationName));

  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = owner->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());

  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(soda, flavorOnly));

  auto const region = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      region,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      flavorOnly,
      RegionHandleSet{region},
  }};
  auto const objectInstance = owner->registerObjectInstanceWithRegions(
      soda,
      sourcePair);
  REQUIRE(objectInstance.isValid());

  // Complete discovery so the peer receives the later Remove callback.
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_THROWS_AS(
      owner->deleteRegion(region),
      rti1516_2025::RegionInUseForUpdateOrSubscription);

  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, VariableLengthData{}));
  // The object is retained until the peer consumes its pending removal, so
  // the source region remains protected at this boundary.
  REQUIRE_THROWS_AS(
      owner->deleteRegion(region),
      rti1516_2025::RegionInUseForUpdateOrSubscription);

  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectRemovalReports.size() == 1U);
  // The final removal callback purges the execution-wide object and must
  // refresh the region ledger before returning.
  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_THROWS_AS(
      owner->getDimensionHandleSet(region),
      rti1516_2025::InvalidRegion);

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded committed receiver region discovers an existing regional object when it enters overlap",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[region-lifecycle][region-commit-discovery]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.register-object-instance-with-regions]"
    "[federate.callback.discover-object-instance]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherFederate;
  REQUIRE_NOTHROW(publisherFederate = publisher->joinFederationExecution(
      L"region-commit-discovery-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"region-commit-discovery-receiver",
      L"subscriber",
      federationName));

  // Keep declaration-relevance advisories out of this callback-focused slice;
  // the regional subscription remains active and is the only discovery input.
  suppressDeclarationRelevanceAdvisories(*publisher);
  suppressDeclarationRelevanceAdvisories(*receiver);

  auto const soda = publisher->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));

  auto const sourceRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));

  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      flavorOnly,
      RegionHandleSet{sourceRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.empty());

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      sourcePair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.empty());

  // The object is already registered, but the receiver's committed range is
  // disjoint. Mutating and committing that same region is the discovery
  // boundary; no new subscription or registration event is involved.
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  auto const& discovery = receiverReports.objectDiscoveryReports.front();
  REQUIRE(discovery.objectInstance == objectInstance);
  REQUIRE(discovery.objectClass == soda);
  REQUIRE(discovery.objectInstanceName == publisher->getObjectInstanceName(objectInstance));
  REQUIRE(discovery.producingFederate == publisherFederate);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, sourcePair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded existing regional object is discovered when an overlapping update region is associated",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[region-lifecycle][association-discovery]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[federate.callback.discover-object-instance]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherFederate;
  REQUIRE_NOTHROW(publisherFederate = publisher->joinFederationExecution(
      L"association-discovery-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"association-discovery-receiver",
      L"subscriber",
      federationName));

  suppressDeclarationRelevanceAdvisories(*publisher);
  suppressDeclarationRelevanceAdvisories(*receiver);
  auto const soda = publisher->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));

  auto const sourceRegionA = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const sourceRegionB = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegionA,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegionB,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceRegionA, sourceRegionB}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));

  AttributeHandleSetRegionHandleSetPairVector const initialSourcePair{{
      flavorOnly,
      RegionHandleSet{sourceRegionA},
  }};
  AttributeHandleSetRegionHandleSetPairVector const addedSourcePair{{
      flavorOnly,
      RegionHandleSet{sourceRegionB},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.empty());

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      initialSourcePair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.empty());

  // Registration remains disjoint. Adding the second source realization is
  // the boundary that makes this already-registered object discoverable.
  REQUIRE_NOTHROW(publisher->associateRegionsForUpdates(objectInstance, addedSourcePair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  auto const& discovery = receiverReports.objectDiscoveryReports.front();
  REQUIRE(discovery.objectInstance == objectInstance);
  REQUIRE(discovery.objectClass == soda);
  REQUIRE(discovery.objectInstanceName == publisher->getObjectInstanceName(objectInstance));
  REQUIRE(discovery.producingFederate == publisherFederate);

  // Removing the only source association restores the default source region.
  // A separately registered object therefore crosses the same discovery
  // boundary without a new receiver declaration.
  ObjectInstanceHandle defaultedObject;
  REQUIRE_NOTHROW(defaultedObject = publisher->registerObjectInstanceWithRegions(
      soda,
      initialSourcePair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(
      defaultedObject,
      initialSourcePair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 2U);
  auto const& defaultedDiscovery = receiverReports.objectDiscoveryReports.back();
  REQUIRE(defaultedDiscovery.objectInstance == defaultedObject);
  REQUIRE(defaultedDiscovery.objectClass == soda);
  REQUIRE(defaultedDiscovery.producingFederate == publisherFederate);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, addedSourcePair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, initialSourcePair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegionB));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegionA));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
