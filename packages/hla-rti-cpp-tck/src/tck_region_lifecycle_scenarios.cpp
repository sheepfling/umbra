#include "tck_main_scenarios.hpp"
#include "tck_region_lifecycle_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {


void verifyDdmRegion(
    Session& session,
    DdmHandles const& handles,
    rti::RegionHandle const& region,
    unsigned long firstLower,
    unsigned long firstUpper,
    unsigned long secondLower,
    unsigned long secondUpper,
    std::string const& description) {
  auto& rtiAmbassador = session.rtiAmbassador();
  auto const dimensions = rtiAmbassador.getDimensionHandleSet(region);
  require(
      dimensions.count(handles.firstDimension) == 1U &&
          dimensions.count(handles.secondDimension) == 1U && dimensions.size() == 2U,
      description + " returned the wrong region dimension set");
  auto const firstBounds =
      rtiAmbassador.getRangeBounds(region, handles.firstDimension);
  auto const secondBounds =
      rtiAmbassador.getRangeBounds(region, handles.secondDimension);
  require(
      firstBounds.getLowerBound() == firstLower &&
          firstBounds.getUpperBound() == firstUpper &&
          secondBounds.getLowerBound() == secondLower &&
          secondBounds.getUpperBound() == secondUpper,
      description + " returned the wrong range bounds");
}


rti::DimensionHandle ddmUnrelatedDimensionHandle(
    Session& session,
    Options const& options) {
  auto const dimension = session.rtiAmbassador().getDimensionHandle(
      options.ddmUnrelatedDimensionName);
  require(
      dimension.isValid(),
      "DDM adapter supplied an invalid unrelated dimension handle");
  return dimension;
}

void scenarioRegionLifecycle(Options const& options, rti::CallbackModel model) {
  Session lifecycle(options, model, "region-lifecycle-boundary");
  rti::ObjectClassHandle const invalidRegionObjectClass;
  rti::InteractionClassHandle const invalidRegionInteractionClass;
  rti::ObjectInstanceHandle const invalidRegionObjectInstance;
  rti::DimensionHandle const invalidRegionDimension;
  rti::DimensionHandleSet const emptyDimensions;
  rti::RegionHandle const invalidRegion;
  rti::RegionHandleSet const emptyRegions;
  rti::AttributeHandleSetRegionHandleSetPairVector const emptyAttributeRegionPairs;
  rti::ParameterHandleValueMap const emptyRegionParameters;
  rti::VariableLengthData const emptyRegionTag;
  rti::RangeBounds const lifecycleRangeBounds(0UL, 1UL);
  auto checkRegionServiceLifecycle = [&](std::wstring const& expected,
                                          std::string const& phase) {
    requireException(
        [&] { static_cast<void>(lifecycle.rtiAmbassador().createRegion(emptyDimensions)); },
        expected,
        "region creation " + phase);
    requireException(
        [&] { lifecycle.rtiAmbassador().commitRegionModifications(emptyRegions); },
        expected,
        "region commit " + phase);
    requireException(
        [&] { lifecycle.rtiAmbassador().deleteRegion(invalidRegion); },
        expected,
        "region deletion " + phase);
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().registerObjectInstanceWithRegions(
              invalidRegionObjectClass,
              emptyAttributeRegionPairs));
        },
        expected,
        "region-qualified object registration " + phase);
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().registerObjectInstanceWithRegions(
              invalidRegionObjectClass,
              emptyAttributeRegionPairs,
              L"tck-region-lifecycle-object"));
        },
        expected,
        "named region-qualified object registration " + phase);
    requireException(
        [&] {
          lifecycle.rtiAmbassador().associateRegionsForUpdates(
              invalidRegionObjectInstance,
              emptyAttributeRegionPairs);
        },
        expected,
        "region association " + phase);
    requireException(
        [&] {
          lifecycle.rtiAmbassador().unassociateRegionsForUpdates(
              invalidRegionObjectInstance,
              emptyAttributeRegionPairs);
        },
        expected,
        "region unassociation " + phase);
    requireException(
        [&] {
          lifecycle.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
              invalidRegionObjectClass,
              emptyAttributeRegionPairs,
              false,
              L"");
        },
        expected,
        "regional attribute subscription " + phase);
    requireException(
        [&] {
          lifecycle.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
              invalidRegionObjectClass,
              emptyAttributeRegionPairs);
        },
        expected,
        "regional attribute unsubscription " + phase);
    requireException(
        [&] {
          lifecycle.rtiAmbassador().subscribeInteractionClassWithRegions(
              invalidRegionInteractionClass,
              emptyRegions,
              false);
        },
        expected,
        "regional interaction subscription " + phase);
    requireException(
        [&] {
          lifecycle.rtiAmbassador().unsubscribeInteractionClassWithRegions(
              invalidRegionInteractionClass,
              emptyRegions);
        },
        expected,
        "regional interaction unsubscription " + phase);
    requireException(
        [&] {
          lifecycle.rtiAmbassador().sendInteractionWithRegions(
              invalidRegionInteractionClass,
              emptyRegionParameters,
              emptyRegions,
              emptyRegionTag);
        },
        expected,
        "regional interaction send " + phase);
    requireException(
        [&] {
          lifecycle.rtiAmbassador().requestAttributeValueUpdateWithRegions(
              invalidRegionObjectClass,
              emptyAttributeRegionPairs,
              emptyRegionTag);
        },
        expected,
        "regional attribute update request " + phase);
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().getRangeBounds(
              invalidRegion,
              invalidRegionDimension));
        },
        expected,
        "range lookup " + phase);
    requireException(
        [&] {
          lifecycle.rtiAmbassador().setRangeBounds(
              invalidRegion,
              invalidRegionDimension,
              lifecycleRangeBounds);
        },
        expected,
        "range update " + phase);
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().decodeRegionHandle(
              rti::VariableLengthData{}));
        },
        expected,
        "region-handle decoding " + phase);
    requireException(
        [&] { static_cast<void>(lifecycle.rtiAmbassador().getDimensionHandleSet(invalidRegion)); },
        expected,
        "dimension lookup for a region " + phase);
  };
  checkRegionServiceLifecycle(L"NotConnected", "before connect");
  lifecycle.connect();
  checkRegionServiceLifecycle(L"FederateNotExecutionMember", "before joining an execution");
  lifecycle.disconnect();

  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "region-lifecycle");
  connectAndJoin(owner, member, options, federation, options.ddmFom);

  auto lifecycleTime = makeTimeContext(owner);
  requireException(
      [&] {
        static_cast<void>(lifecycle.rtiAmbassador().sendInteractionWithRegions(
            invalidRegionInteractionClass,
            emptyRegionParameters,
            emptyRegions,
            emptyRegionTag,
            *lifecycleTime.initial));
      },
      L"NotConnected",
      "timestamped regional interaction send before connect");
  lifecycle.connect();
  requireException(
      [&] {
        static_cast<void>(lifecycle.rtiAmbassador().sendInteractionWithRegions(
            invalidRegionInteractionClass,
            emptyRegionParameters,
            emptyRegions,
            emptyRegionTag,
            *lifecycleTime.initial));
      },
      L"FederateNotExecutionMember",
      "timestamped regional interaction send before joining an execution");
  lifecycle.disconnect();

  auto const ownerHandles = ddmHandles(owner, options);
  auto const memberHandles = ddmHandles(member, options);
  verifyDdmClassDimensions(owner, options, ownerHandles, "region lifecycle");
  verifyDdmClassDimensions(member, options, memberHandles, "region lifecycle member");
  require(
      owner.rtiAmbassador().getDimensionUpperBound(ownerHandles.firstDimension) ==
              member.rtiAmbassador().getDimensionUpperBound(memberHandles.firstDimension) &&
          owner.rtiAmbassador().getDimensionUpperBound(ownerHandles.secondDimension) ==
              member.rtiAmbassador().getDimensionUpperBound(memberHandles.secondDimension),
      "region lifecycle members reported different dimension upper bounds");

  auto const ownerRegion = createDdmRegion(owner, ownerHandles, 1UL, 5UL, 2UL, 7UL);
  auto const memberRegion = createDdmRegion(member, memberHandles, 1UL, 5UL, 2UL, 7UL);
  verifyDdmRegion(
      owner,
      ownerHandles,
      ownerRegion,
      1UL,
      5UL,
      2UL,
      7UL,
      "owner region metadata");
  verifyDdmRegion(
      member,
      memberHandles,
      memberRegion,
      1UL,
      5UL,
      2UL,
      7UL,
      "member region metadata");

  owner.rtiAmbassador().setRangeBounds(
      ownerRegion,
      ownerHandles.firstDimension,
      rti::RangeBounds(0UL, 4UL));
  owner.rtiAmbassador().commitRegionModifications(rti::RegionHandleSet{ownerRegion});
  verifyDdmRegion(
      owner,
      ownerHandles,
      ownerRegion,
      0UL,
      4UL,
      2UL,
      7UL,
      "committed region modification");

  auto const oneDimensionRegion = owner.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{ownerHandles.firstDimension});
  require(oneDimensionRegion.isValid(), "one-dimensional DDM region creation failed");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getRangeBounds(
            oneDimensionRegion, ownerHandles.secondDimension));
      },
      L"RegionDoesNotContainSpecifiedDimension",
      "range lookup for a dimension absent from a region");
  requireException(
      [&] {
        owner.rtiAmbassador().setRangeBounds(
            oneDimensionRegion,
            ownerHandles.secondDimension,
            rti::RangeBounds(0UL, 1UL));
      },
      L"RegionDoesNotContainSpecifiedDimension",
      "range update for a dimension absent from a region");
  owner.rtiAmbassador().deleteRegion(oneDimensionRegion);

  requireException(
      [&] {
        owner.rtiAmbassador().setRangeBounds(
            ownerRegion,
            ownerHandles.firstDimension,
            rti::RangeBounds(4UL, 4UL));
      },
      L"InvalidRangeBound",
      "region range with equal lower and upper bounds");
  requireException(
      [&] {
        owner.rtiAmbassador().setRangeBounds(
            ownerRegion,
            ownerHandles.secondDimension,
            rti::RangeBounds(0UL, owner.rtiAmbassador().getDimensionUpperBound(
                ownerHandles.secondDimension) + 1UL));
      },
      L"InvalidRangeBound",
      "region range beyond the dimension upper bound");

  owner.rtiAmbassador().deleteRegion(ownerRegion);
  member.rtiAmbassador().deleteRegion(memberRegion);
  requireException(
      [&] { static_cast<void>(owner.rtiAmbassador().getDimensionHandleSet(ownerRegion)); },
      L"InvalidRegion",
      "dimension lookup after region deletion");

  owner.resign(rti::NO_ACTION);
  member.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  member.disconnect();
}

void scenarioRegionLifecycleContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionLifecycle(options, model);
}


void scenarioRegionalUnpublishRegionRelease(Options const& options, rti::CallbackModel model) {
  require(!options.ddmFom.empty(),
          "regional unpublish region release requires an adapter-supplied dimensional FOM");

  Session owner(options, model, "regional-unpublish-owner");
  auto const federation = federationName(options, "regional-unpublish-region-release");
  owner.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.ddmFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  auto const handles = ddmHandles(owner, options);
  verifyDdmClassDimensions(owner, options, handles, "regional unpublish region release");

  rti::AttributeHandleSet const attributes{handles.attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      handles.objectClass,
      attributes);

  auto const region = createDdmRegion(owner, handles, 1UL, 5UL, 1UL, 5UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const attributeRegions{{
      attributes,
      rti::RegionHandleSet{region},
  }};
  auto const object = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      handles.objectClass,
      attributeRegions);
  require(object.isValid(),
          "regional unpublish region release returned an invalid object handle");

  requireException(
      [&] { owner.rtiAmbassador().deleteRegion(region); },
      L"RegionInUseForUpdateOrSubscription",
      "deletion of a region used by an object update association");

  owner.rtiAmbassador().unpublishObjectClassAttributes(
      handles.objectClass,
      attributes);
  owner.rtiAmbassador().deleteRegion(region);
  requireException(
      [&] { static_cast<void>(owner.rtiAmbassador().getDimensionHandleSet(region)); },
      L"InvalidRegion",
      "dimension lookup after unpublishing releases the region");

  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
}

void scenarioRegionalUnpublishRegionReleaseContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalUnpublishRegionRelease(options, model);
}

void scenarioRegionalObjectUpdate(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session overlap(options, model, "member");
  Session disjoint(options, model, "passive");
  auto const federation = federationName(options, "regional-object-update");
  connectAndJoin(owner, overlap, options, federation, options.ddmFom);
  disjoint.connect();
  disjoint.join(
      options.memberFederateName + L"-regional-disjoint",
      options.federateType,
      federation);

  auto const ownerHandles = ddmHandles(owner, options);
  auto const overlapHandles = ddmHandles(overlap, options);
  auto const disjointHandles = ddmHandles(disjoint, options);
  verifyDdmClassDimensions(owner, options, ownerHandles, "regional object update");

  rti::AttributeHandleSet const ownerAttributes{ownerHandles.attribute};
  rti::AttributeHandleSet const overlapAttributes{overlapHandles.attribute};
  rti::AttributeHandleSet const disjointAttributes{disjointHandles.attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerHandles.objectClass, ownerAttributes);

  auto const sourceRegion = createDdmRegion(owner, ownerHandles, 1UL, 5UL, 1UL, 5UL);
  auto const disjointSourceRegion =
      createDdmRegion(owner, ownerHandles, 7UL, 9UL, 7UL, 9UL);
  auto const overlapRegion =
      createDdmRegion(overlap, overlapHandles, 2UL, 6UL, 2UL, 6UL);
  auto const disjointRegion =
      createDdmRegion(disjoint, disjointHandles, 7UL, 9UL, 7UL, 9UL);

  rti::AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      ownerAttributes,
      rti::RegionHandleSet{sourceRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const disjointSourcePair{{
      ownerAttributes,
      rti::RegionHandleSet{disjointSourceRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const overlapPair{{
      overlapAttributes,
      rti::RegionHandleSet{overlapRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const disjointPair{{
      disjointAttributes,
      rti::RegionHandleSet{disjointRegion},
  }};

  overlap.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  disjoint.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(overlap.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
          "overlap receiver did not enable region designator callbacks");
  require(disjoint.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
          "disjoint receiver did not enable region designator callbacks");
  auto verifyTransportation = [&](Session& receiver,
                                  rti::TransportationTypeHandle const& transportation,
                                  std::string const& description) {
    require(transportation.isValid(), description + " returned an invalid transportation handle");
    if (options.requireCustomTransportation) {
      require(
          receiver.rtiAmbassador().getTransportationTypeName(transportation) ==
              options.fomCustomTransportationName,
          description + " changed the adapter-declared custom transport");
    }
  };
  overlap.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      overlapHandles.objectClass, overlapPair, true, L"");
  disjoint.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      disjointHandles.objectClass, disjointPair, true, L"");

  auto const objectName = federation + L"-regional-object";
  owner.rtiAmbassador().reserveObjectInstanceName(objectName);
  waitFor(
      owner,
      [&] { return owner.recorder().reservationSucceeded(objectName); },
      options,
      "regional named object reservation");
  auto const object = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerHandles.objectClass,
      sourcePair,
      objectName);
  require(object.isValid(), "regional object registration returned an invalid handle");
  waitFor(
      overlap,
      [&] { return overlap.recorder().hasDiscovery(object); },
      options,
      "regional overlapping object discovery");
  for (int pass = 0; pass != 8; ++pass) {
    disjoint.pump();
  }
  require(!disjoint.recorder().hasDiscovery(object),
          "disjoint regional subscription discovered the object");

  std::vector<std::uint8_t> firstValue{0x11U, 0x22U, 0x33U};
  std::vector<std::uint8_t> firstTag{0x52U, 0x31U};
  rti::AttributeHandleValueMap firstValues;
  firstValues.emplace(
      ownerHandles.attribute,
      rti::VariableLengthData(firstValue.data(), firstValue.size()));
  rti::VariableLengthData firstUserTag(firstTag.data(), firstTag.size());
  owner.rtiAmbassador().updateAttributeValues(object, firstValues, firstUserTag);
  waitFor(
      overlap,
      [&] { return overlap.recorder().reflection().present; },
      options,
      "regional ordinary attribute reflection");
  auto const firstReflection = overlap.recorder().reflection();
  require(firstReflection.object == object &&
              firstReflection.values.size() == 1U &&
              firstReflection.values.count(overlapHandles.attribute) == 1U,
          "regional reflection returned the wrong object or attribute set");
  require(copyBytes(firstReflection.values.at(overlapHandles.attribute)) == firstValue,
          "regional reflection returned the wrong attribute value");
  require(firstReflection.tag == firstTag &&
              firstReflection.producer == owner.federateHandle() &&
              firstReflection.transportation.isValid(),
          "regional reflection did not preserve ordinary metadata");
  verifyTransportation(overlap, firstReflection.transportation, "regional reflection");
  require(firstReflection.regions.has_value() &&
              firstReflection.regions->size() == 1U &&
              firstReflection.regions->count(sourceRegion) == 1U,
          "regional reflection did not convey the source region designator set");
  for (int pass = 0; pass != 8; ++pass) {
    disjoint.pump();
  }
  require(!disjoint.recorder().discovery().present &&
              !disjoint.recorder().reflection().present,
          "disjoint regional subscription received the initial update");

  std::vector<std::uint8_t> requestTagBytes{0x52U, 0x51U};
  rti::VariableLengthData requestTag(requestTagBytes.data(), requestTagBytes.size());
  overlap.recorder().clearReflection();
  owner.recorder().clearProvidedUpdates();
  overlap.rtiAmbassador().requestAttributeValueUpdateWithRegions(
      overlapHandles.objectClass,
      overlapPair,
      requestTag);
  waitFor(
      owner,
      [&] {
        auto const requests = owner.recorder().providedUpdates();
        return !requests.empty() && requests.back().object == object &&
            requests.back().attributes == ownerAttributes &&
            requests.back().tag == requestTagBytes;
      },
      options,
      "regional attribute-value request callback");
  auto const requests = owner.recorder().providedUpdates();
  require(requests.back().object == object &&
              requests.back().attributes == ownerAttributes &&
              requests.back().tag == requestTagBytes,
          "regional attribute-value request returned the wrong callback data");
  std::vector<std::uint8_t> responseValue{0x44U, 0x55U, 0x66U};
  std::vector<std::uint8_t> responseTag{0x52U, 0x53U};
  rti::AttributeHandleValueMap responseValues;
  responseValues.emplace(
      ownerHandles.attribute,
      rti::VariableLengthData(responseValue.data(), responseValue.size()));
  rti::VariableLengthData responseUserTag(responseTag.data(), responseTag.size());
  owner.rtiAmbassador().updateAttributeValues(object, responseValues, responseUserTag);
  waitFor(
      overlap,
      [&] { return overlap.recorder().reflection().present; },
      options,
      "regional attribute-value response reflection");
  auto const responseReflection = overlap.recorder().reflection();
  require(copyBytes(responseReflection.values.at(overlapHandles.attribute)) == responseValue &&
              responseReflection.tag == responseTag,
          "regional attribute-value response did not preserve data");
  verifyTransportation(
      overlap,
      responseReflection.transportation,
      "regional attribute-value response reflection");

  owner.rtiAmbassador().associateRegionsForUpdates(object, disjointSourcePair);
  owner.rtiAmbassador().unassociateRegionsForUpdates(object, sourcePair);
  waitFor(
      disjoint,
      [&] { return disjoint.recorder().hasDiscovery(object); },
      options,
      "regional disjoint object discovery after association");
  overlap.recorder().clearReflection();
  disjoint.recorder().clearReflection();
  std::vector<std::uint8_t> disjointValue{0x77U, 0x88U};
  rti::AttributeHandleValueMap disjointValues;
  disjointValues.emplace(
      ownerHandles.attribute,
      rti::VariableLengthData(disjointValue.data(), disjointValue.size()));
  owner.rtiAmbassador().updateAttributeValues(object, disjointValues, responseUserTag);
  waitFor(
      disjoint,
      [&] { return disjoint.recorder().reflection().present; },
      options,
      "regional associated attribute reflection");
  auto const associatedReflection = disjoint.recorder().reflection();
  require(copyBytes(associatedReflection.values.at(disjointHandles.attribute)) ==
              disjointValue,
          "associated regional reflection returned the wrong value");
  verifyTransportation(
      disjoint,
      associatedReflection.transportation,
      "associated regional reflection");
  for (int pass = 0; pass != 8; ++pass) {
    overlap.pump();
  }
  require(!overlap.recorder().reflection().present,
          "overlap receiver received an update after source-region reassociation");

  owner.rtiAmbassador().unassociateRegionsForUpdates(object, disjointSourcePair);
  overlap.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      overlapHandles.objectClass, overlapPair);
  disjoint.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      disjointHandles.objectClass, disjointPair);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerHandles.objectClass, ownerAttributes);
  owner.rtiAmbassador().deleteRegion(sourceRegion);
  owner.rtiAmbassador().deleteRegion(disjointSourceRegion);
  overlap.rtiAmbassador().deleteRegion(overlapRegion);
  disjoint.rtiAmbassador().deleteRegion(disjointRegion);

  owner.resign(rti::DELETE_OBJECTS);
  overlap.resign(rti::NO_ACTION);
  disjoint.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  overlap.disconnect();
  disjoint.disconnect();
}

void scenarioRegionalObjectUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalObjectUpdate(options, model);
}


void scenarioRegionalAttributeValueUpdateResponseRecheck(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Regional attribute-value response recheck requires an adapter-supplied dimensional FOM");

  Session owner(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(
      options,
      "regional-attribute-value-update-response-recheck");
  connectAndJoin(owner, requester, options, federation, options.ddmFom);

  auto const ownerHandles = ddmHandles(owner, options);
  auto const requesterHandles = ddmHandles(requester, options);
  verifyDdmClassDimensions(
      owner,
      options,
      ownerHandles,
      "regional attribute-value response recheck");

  rti::AttributeHandleSet const ownerAttributes{ownerHandles.attribute};
  rti::AttributeHandleSet const requesterAttributes{requesterHandles.attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerHandles.objectClass,
      ownerAttributes);

  auto const ownerRegion = createDdmRegion(
      owner,
      ownerHandles,
      0UL,
      1UL,
      0UL,
      1UL);
  auto const requesterRegion = createDdmRegion(
      requester,
      requesterHandles,
      0UL,
      1UL,
      0UL,
      1UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      ownerAttributes,
      rti::RegionHandleSet{ownerRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const requesterPair{{
      requesterAttributes,
      rti::RegionHandleSet{requesterRegion},
  }};

  requester.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  requester.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      requesterHandles.objectClass,
      requesterPair,
      true,
      L"");
  auto const object = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerHandles.objectClass,
      ownerPair);
  require(
      object.isValid(),
      "regional attribute-value response registration returned an invalid object handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "regional attribute-value response discovery");

  std::vector<std::uint8_t> const requestTagBytes{0x52U, 0x45U, 0x51U};
  std::vector<std::uint8_t> const responseValueBytes{0x5AU, 0x25U, 0x07U};
  std::vector<std::uint8_t> const responseTagBytes{0x52U, 0x45U, 0x53U};
  rti::VariableLengthData const requestTag(
      requestTagBytes.data(),
      requestTagBytes.size());
  rti::VariableLengthData const responseTag(
      responseTagBytes.data(),
      responseTagBytes.size());
  rti::AttributeHandleValueMap responseValues;
  responseValues.emplace(
      ownerHandles.attribute,
      rti::VariableLengthData(
          responseValueBytes.data(),
          responseValueBytes.size()));

  owner.recorder().clearProvidedUpdates();
  requester.recorder().clearReflection();
  requester.rtiAmbassador().requestAttributeValueUpdateWithRegions(
      requesterHandles.objectClass,
      requesterPair,
      requestTag);
  waitFor(
      owner,
      [&] { return owner.recorder().providedUpdates().size() == 1U; },
      options,
      "regional attribute-value response provider callback");
  auto const requests = owner.recorder().providedUpdates();
  require(
      requests.front().object == object &&
          requests.front().attributes == ownerAttributes &&
          requests.front().tag == requestTagBytes,
      "regional attribute-value response provider callback returned the wrong metadata");

  // The response is sent after the requester moves its committed region out of
  // overlap. Reflection delivery must re-check the current DDM eligibility.
  requester.rtiAmbassador().setRangeBounds(
      requesterRegion,
      requesterHandles.firstDimension,
      rti::RangeBounds(2UL, 3UL));
  requester.rtiAmbassador().setRangeBounds(
      requesterRegion,
      requesterHandles.secondDimension,
      rti::RangeBounds(2UL, 3UL));
  requester.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{requesterRegion});
  owner.rtiAmbassador().updateAttributeValues(
      object,
      responseValues,
      responseTag);
  for (int pass = 0; pass != 8; ++pass) {
    requester.pump();
  }
  require(
      requester.recorder().reflectionCount() == 0U,
      "regional attribute-value response was reflected after overlap was removed");

  // Restore overlap and prove that a fresh request/response is delivered with
  // the ordinary value, tag, transport, and producer metadata intact.
  requester.rtiAmbassador().setRangeBounds(
      requesterRegion,
      requesterHandles.firstDimension,
      rti::RangeBounds(0UL, 1UL));
  requester.rtiAmbassador().setRangeBounds(
      requesterRegion,
      requesterHandles.secondDimension,
      rti::RangeBounds(0UL, 1UL));
  requester.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{requesterRegion});
  owner.recorder().clearProvidedUpdates();
  requester.recorder().clearReflection();
  requester.rtiAmbassador().requestAttributeValueUpdateWithRegions(
      requesterHandles.objectClass,
      requesterPair,
      requestTag);
  waitFor(
      owner,
      [&] { return owner.recorder().providedUpdates().size() == 1U; },
      options,
      "regional attribute-value response restoration callback");
  owner.rtiAmbassador().updateAttributeValues(
      object,
      responseValues,
      responseTag);
  waitFor(
      requester,
      [&] { return requester.recorder().reflection().present; },
      options,
      "regional attribute-value response restoration reflection");
  auto const reflection = requester.recorder().reflection();
  require(
      reflection.object == object &&
          reflection.values.size() == 1U &&
          reflection.values.count(requesterHandles.attribute) == 1U &&
          copyBytes(reflection.values.at(requesterHandles.attribute)) ==
              responseValueBytes &&
          reflection.tag == responseTagBytes,
      "regional attribute-value response restoration returned the wrong value or tag");
  require(
      reflection.transportation ==
              requester.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable") &&
          reflection.producer == owner.federateHandle(),
      "regional attribute-value response restoration returned the wrong delivery metadata");

  requester.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      requesterHandles.objectClass,
      requesterPair);
  owner.rtiAmbassador().unassociateRegionsForUpdates(object, ownerPair);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerHandles.objectClass,
      ownerAttributes);
  requester.rtiAmbassador().deleteRegion(requesterRegion);
  owner.rtiAmbassador().deleteRegion(ownerRegion);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioRegionalAttributeValueUpdateResponseRecheckContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalAttributeValueUpdateResponseRecheck(options, model);
}

void scenarioRegionalAttributeUpdateCallbackDdmRecheck(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Regional attribute callback DDM recheck requires an adapter-supplied dimensional FOM");

  Session publisher(options, model, "publisher");
  Session active(options, model, "active");
  Session rechecked(options, model, "rechecked");
  auto const federation = federationName(
      options,
      "regional-attribute-update-callback-ddm-recheck");
  connectAndJoin(publisher, active, options, federation, options.ddmFom);
  rechecked.connect();
  rechecked.join(
      options.memberFederateName + L"-regional-rechecked",
      options.federateType,
      federation);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const activeHandles = ddmHandles(active, options);
  auto const recheckedHandles = ddmHandles(rechecked, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "regional attribute callback DDM publisher");

  rti::AttributeHandleSet const publisherAttributes{
      publisherHandles.attribute};
  rti::AttributeHandleSet const activeAttributes{activeHandles.attribute};
  rti::AttributeHandleSet const recheckedAttributes{
      recheckedHandles.attribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);

  auto const sourceRegion = createDdmRegion(
      publisher,
      publisherHandles,
      1UL,
      5UL,
      1UL,
      5UL);
  auto const activeRegion = createDdmRegion(
      active,
      activeHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const recheckedRegion = createDdmRegion(
      rechecked,
      recheckedHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      publisherAttributes,
      rti::RegionHandleSet{sourceRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const activePair{{
      activeAttributes,
      rti::RegionHandleSet{activeRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const recheckedPair{{
      recheckedAttributes,
      rti::RegionHandleSet{recheckedRegion},
  }};

  active.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  rechecked.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  active.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      activeHandles.objectClass,
      activePair,
      true,
      L"");
  rechecked.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      recheckedHandles.objectClass,
      recheckedPair,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
      publisherHandles.objectClass,
      sourcePair);
  require(
      object.isValid(),
      "regional attribute callback DDM registration returned an invalid object handle");
  waitForSessions(
      {&active, &rechecked},
      [&] {
        return active.recorder().hasDiscovery(object) &&
            rechecked.recorder().hasDiscovery(object);
      },
      options,
      "regional attribute callback DDM discovery");

  auto sendUpdate = [&](std::vector<std::uint8_t> value,
                        std::vector<std::uint8_t> tag) {
    rti::AttributeHandleValueMap values;
    values.emplace(
        publisherHandles.attribute,
        rti::VariableLengthData(value.data(), value.size()));
    publisher.rtiAmbassador().updateAttributeValues(
        object,
        values,
        rti::VariableLengthData(tag.data(), tag.size()));
  };
  auto moveRecheckedRegion = [&](bool overlap) {
    auto const bounds = overlap
        ? rti::RangeBounds(2UL, 6UL)
        : rti::RangeBounds(7UL, 9UL);
    rechecked.rtiAmbassador().setRangeBounds(
        recheckedRegion,
        recheckedHandles.firstDimension,
        bounds);
    rechecked.rtiAmbassador().setRangeBounds(
        recheckedRegion,
        recheckedHandles.secondDimension,
        bounds);
    rechecked.rtiAmbassador().commitRegionModifications(
        rti::RegionHandleSet{recheckedRegion});
  };
  auto requireReflection = [&](Session& receiver,
                               rti::AttributeHandle const& attribute,
                               std::vector<std::uint8_t> const& value,
                               std::vector<std::uint8_t> const& tag,
                               std::string const& description) {
    auto const reflection = receiver.recorder().reflection();
    auto const reliable = receiver.rtiAmbassador().getTransportationTypeHandle(
        L"HLAreliable");
    require(
        reliable.isValid(),
        description + " could not resolve HLAreliable");
    require(
        reflection.present &&
            reflection.object == object &&
            reflection.values.size() == 1U &&
            reflection.values.count(attribute) == 1U &&
            copyBytes(reflection.values.at(attribute)) == value &&
            reflection.tag == tag &&
            reflection.transportation == reliable &&
            reflection.producer == publisher.federateHandle(),
        description + " returned the wrong object, value, or delivery metadata");
    require(
        reflection.regions.has_value() &&
            reflection.regions->size() == 1U &&
            reflection.regions->count(sourceRegion) == 1U,
        description + " omitted the source region designator");
  };

  active.recorder().clearReflection();
  rechecked.recorder().clearReflection();
  std::vector<std::uint8_t> const firstValue{0x41U, 0x31U};
  std::vector<std::uint8_t> const firstTag{0x52U, 0x31U};
  sendUpdate(firstValue, firstTag);
  if (model == rti::HLA_EVOKED) {
    require(
        active.recorder().reflectionCount() == 0U &&
            rechecked.recorder().reflectionCount() == 0U,
        "regional attribute callback DDM update delivered before callback servicing");
    moveRecheckedRegion(false);
    waitFor(
        active,
        [&] { return active.recorder().reflectionCount() >= 1U; },
        options,
        "regional attribute callback DDM active reflection");
    for (int pass = 0; pass != 8; ++pass) {
      rechecked.pump();
    }
    require(
        rechecked.recorder().reflectionCount() == 0U,
        "regional attribute callback DDM delivered a stale disjoint reflection");
  } else {
    waitFor(
        active,
        [&] { return active.recorder().reflectionCount() >= 1U; },
        options,
        "immediate regional attribute callback DDM active reflection");
    waitFor(
        rechecked,
        [&] { return rechecked.recorder().reflectionCount() >= 1U; },
        options,
        "immediate regional attribute callback DDM rechecked reflection");
    moveRecheckedRegion(false);
  }
  requireReflection(
      active,
      activeHandles.attribute,
      firstValue,
      firstTag,
      "regional attribute callback DDM active reflection");
  if (model == rti::HLA_IMMEDIATE) {
    requireReflection(
        rechecked,
        recheckedHandles.attribute,
        firstValue,
        firstTag,
        "immediate regional attribute callback DDM rechecked reflection");
  }
  require(
      publisher.recorder().reflectionCount() == 0U,
      "regional attribute callback DDM looped a reflection to the publisher");

  active.recorder().clearReflection();
  rechecked.recorder().clearReflection();
  std::vector<std::uint8_t> const secondValue{0x41U, 0x32U};
  std::vector<std::uint8_t> const secondTag{0x52U, 0x32U};
  sendUpdate(secondValue, secondTag);
  waitFor(
      active,
      [&] { return active.recorder().reflectionCount() >= 1U; },
      options,
      "regional attribute callback DDM surviving reflection");
  for (int pass = 0; pass != 8; ++pass) {
    rechecked.pump();
  }
  require(
      rechecked.recorder().reflectionCount() == 0U,
      "regional attribute callback DDM delivered while the region was disjoint");
  requireReflection(
      active,
      activeHandles.attribute,
      secondValue,
      secondTag,
      "regional attribute callback DDM surviving reflection");

  moveRecheckedRegion(true);
  active.recorder().clearReflection();
  rechecked.recorder().clearReflection();
  std::vector<std::uint8_t> const restoredValue{0x41U, 0x33U};
  std::vector<std::uint8_t> const restoredTag{0x52U, 0x33U};
  sendUpdate(restoredValue, restoredTag);
  waitFor(
      active,
      [&] { return active.recorder().reflectionCount() >= 1U; },
      options,
      "regional attribute callback DDM restored active reflection");
  waitFor(
      rechecked,
      [&] { return rechecked.recorder().reflectionCount() >= 1U; },
      options,
      "regional attribute callback DDM restored rechecked reflection");
  requireReflection(
      active,
      activeHandles.attribute,
      restoredValue,
      restoredTag,
      "regional attribute callback DDM restored active reflection");
  requireReflection(
      rechecked,
      recheckedHandles.attribute,
      restoredValue,
      restoredTag,
      "regional attribute callback DDM restored rechecked reflection");

  active.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      activeHandles.objectClass,
      activePair);
  rechecked.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      recheckedHandles.objectClass,
      recheckedPair);
  publisher.rtiAmbassador().unassociateRegionsForUpdates(object, sourcePair);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  active.rtiAmbassador().deleteRegion(activeRegion);
  rechecked.rtiAmbassador().deleteRegion(recheckedRegion);
  publisher.rtiAmbassador().deleteRegion(sourceRegion);
  active.resign(rti::NO_ACTION);
  rechecked.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  active.disconnect();
  rechecked.disconnect();
  publisher.disconnect();
}

void scenarioRegionalAttributeUpdateCallbackDdmRecheckContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalAttributeUpdateCallbackDdmRecheck(options, model);
}

void scenarioRegionalAttributeValueRequestFiltering(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Regional attribute-value request filtering requires an adapter-supplied dimensional FOM");

  Session owner(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(
      options,
      "regional-attribute-value-request-filtering");
  connectAndJoin(owner, requester, options, federation, options.ddmFom);

  auto const ownerHandles = ddmHandles(owner, options);
  auto const requesterHandles = ddmHandles(requester, options);
  verifyDdmClassDimensions(
      owner,
      options,
      ownerHandles,
      "regional attribute-value request filtering");

  rti::AttributeHandleSet const ownerAttributes{ownerHandles.attribute};
  rti::AttributeHandleSet const requesterAttributes{requesterHandles.attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerHandles.objectClass,
      ownerAttributes);

  auto const ownerRegion = createDdmRegion(
      owner,
      ownerHandles,
      0UL,
      1UL,
      0UL,
      1UL);
  auto const requestRegion = createDdmRegion(
      requester,
      requesterHandles,
      2UL,
      3UL,
      2UL,
      3UL);

  // A committed region over a dimension outside the object's available set is
  // valid as a region, but is invalid as a request region for this class.
  auto const wrongContextDimension = ddmUnrelatedDimensionHandle(requester, options);
  auto const wrongContextRegion = requester.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{wrongContextDimension});
  require(
      wrongContextRegion.isValid(),
      "incompatible request region creation returned an invalid handle");
  requester.rtiAmbassador().setRangeBounds(
      wrongContextRegion,
      wrongContextDimension,
      rti::RangeBounds(0UL, 1UL));
  requester.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{wrongContextRegion});

  // Leave both required ranges unset so the service must reject the region
  // before attempting to solicit any provider callbacks.
  auto const uncommittedRegion = requester.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{
          requesterHandles.firstDimension,
          requesterHandles.secondDimension});
  require(
      uncommittedRegion.isValid(),
      "uncommitted request region creation returned an invalid handle");

  rti::AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      ownerAttributes,
      rti::RegionHandleSet{ownerRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const disjointRequestPair{{
      requesterAttributes,
      rti::RegionHandleSet{requestRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const emptyRequestPair{{
      requesterAttributes,
      rti::RegionHandleSet{},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const foreignRequestPair{{
      requesterAttributes,
      rti::RegionHandleSet{ownerRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const wrongContextRequestPair{{
      requesterAttributes,
      rti::RegionHandleSet{wrongContextRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const uncommittedRequestPair{{
      requesterAttributes,
      rti::RegionHandleSet{uncommittedRegion},
  }};

  auto const regionalObject = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerHandles.objectClass,
      ownerPair);
  auto const defaultObject = owner.rtiAmbassador().registerObjectInstance(
      ownerHandles.objectClass);
  require(
      regionalObject.isValid() && defaultObject.isValid(),
      "regional request filtering registration returned an invalid object handle");

  std::vector<std::uint8_t> const requestTagBytes{0x91U, 0x25U};
  rti::VariableLengthData const requestTag(
      requestTagBytes.data(),
      requestTagBytes.size());
  requireException(
      [&] {
        requester.rtiAmbassador().requestAttributeValueUpdateWithRegions(
            requesterHandles.objectClass,
            foreignRequestPair,
            requestTag);
      },
      L"RegionNotCreatedByThisFederate",
      "regional value request with a foreign region");
  requireException(
      [&] {
        requester.rtiAmbassador().requestAttributeValueUpdateWithRegions(
            requesterHandles.objectClass,
            wrongContextRequestPair,
            requestTag);
      },
      L"InvalidRegionContext",
      "regional value request with an incompatible region context");
  requireException(
      [&] {
        requester.rtiAmbassador().requestAttributeValueUpdateWithRegions(
            requesterHandles.objectClass,
            uncommittedRequestPair,
            requestTag);
      },
      L"InvalidRegion",
      "regional value request with an uncommitted region");

  owner.recorder().clearProvidedUpdates();
  requester.rtiAmbassador().requestAttributeValueUpdateWithRegions(
      requesterHandles.objectClass,
      emptyRequestPair,
      requestTag);
  for (int pass = 0; pass != 8; ++pass) {
    owner.pump();
  }
  require(
      owner.recorder().providedUpdates().empty(),
      "an empty regional request set generated a provider callback");

  // The explicit [0,1) association is disjoint from [2,3), but the ordinary
  // default-region object remains eligible for a regional request.
  requester.rtiAmbassador().requestAttributeValueUpdateWithRegions(
      requesterHandles.objectClass,
      disjointRequestPair,
      requestTag);
  waitFor(
      owner,
      [&] { return owner.recorder().providedUpdates().size() >= 1U; },
      options,
      "disjoint regional value request callback");
  auto const disjointRequests = owner.recorder().providedUpdates();
  require(
      disjointRequests.size() == 1U &&
          disjointRequests.front().object == defaultObject &&
          disjointRequests.front().attributes == ownerAttributes &&
          disjointRequests.front().tag == requestTagBytes,
      "disjoint regional value request did not retain the default-region object");

  // Recommitting the requester region into the source range makes both the
  // explicit and default-region objects eligible.
  requester.rtiAmbassador().setRangeBounds(
      requestRegion,
      requesterHandles.firstDimension,
      rti::RangeBounds(0UL, 1UL));
  requester.rtiAmbassador().setRangeBounds(
      requestRegion,
      requesterHandles.secondDimension,
      rti::RangeBounds(0UL, 1UL));
  requester.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{requestRegion});
  requester.rtiAmbassador().requestAttributeValueUpdateWithRegions(
      requesterHandles.objectClass,
      disjointRequestPair,
      requestTag);
  waitFor(
      owner,
      [&] { return owner.recorder().providedUpdates().size() >= 3U; },
      options,
      "overlapping regional value request callbacks");
  auto const overlappingRequests = owner.recorder().providedUpdates();
  require(
      overlappingRequests.size() == 3U,
      "overlapping regional value request returned an unexpected callback count");
  bool sawRegionalObject = false;
  bool sawDefaultObject = false;
  for (auto const& request : overlappingRequests) {
    require(
        request.attributes == ownerAttributes && request.tag == requestTagBytes,
        "regional value request callback did not preserve attributes or tag");
    sawRegionalObject = sawRegionalObject || request.object == regionalObject;
    sawDefaultObject = sawDefaultObject || request.object == defaultObject;
  }
  require(
      sawRegionalObject && sawDefaultObject,
      "overlapping regional value request omitted a source object");

  if (model == rti::HLA_EVOKED) {
    // An evoked provider must re-evaluate the request region when the
    // provideAttributeValueUpdate callback is actually delivered.
    requester.rtiAmbassador().requestAttributeValueUpdateWithRegions(
        requesterHandles.objectClass,
        disjointRequestPair,
        requestTag);
    requester.rtiAmbassador().setRangeBounds(
        requestRegion,
        requesterHandles.firstDimension,
        rti::RangeBounds(2UL, 3UL));
    requester.rtiAmbassador().setRangeBounds(
        requestRegion,
        requesterHandles.secondDimension,
        rti::RangeBounds(2UL, 3UL));
    requester.rtiAmbassador().commitRegionModifications(
        rti::RegionHandleSet{requestRegion});
    waitFor(
        owner,
        [&] { return owner.recorder().providedUpdates().size() >= 4U; },
        options,
        "evoked regional value request re-evaluation");
    auto const reevaluatedRequests = owner.recorder().providedUpdates();
    require(
        reevaluatedRequests.size() == 4U &&
            reevaluatedRequests.back().object == defaultObject,
        "evoked regional value request did not suppress the moved explicit source");
  } else {
    // Immediate delivery completes before the request region is moved, so a
    // later region commit must not replay the already-delivered callbacks.
    requester.rtiAmbassador().requestAttributeValueUpdateWithRegions(
        requesterHandles.objectClass,
        disjointRequestPair,
        requestTag);
    waitFor(
        owner,
        [&] { return owner.recorder().providedUpdates().size() >= 5U; },
        options,
        "immediate regional value request callbacks");
    requester.rtiAmbassador().setRangeBounds(
        requestRegion,
        requesterHandles.firstDimension,
        rti::RangeBounds(2UL, 3UL));
    requester.rtiAmbassador().setRangeBounds(
        requestRegion,
        requesterHandles.secondDimension,
        rti::RangeBounds(2UL, 3UL));
    requester.rtiAmbassador().commitRegionModifications(
        rti::RegionHandleSet{requestRegion});
    owner.pump();
    require(
        owner.recorder().providedUpdates().size() == 5U,
        "immediate regional value request was replayed after region movement");
  }

  owner.rtiAmbassador().unassociateRegionsForUpdates(regionalObject, ownerPair);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerHandles.objectClass,
      ownerAttributes);
  owner.rtiAmbassador().deleteRegion(ownerRegion);
  requester.rtiAmbassador().deleteRegion(wrongContextRegion);
  requester.rtiAmbassador().deleteRegion(uncommittedRegion);
  requester.rtiAmbassador().deleteRegion(requestRegion);

  requester.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  requester.disconnect();
}

void scenarioRegionalAttributeValueRequestFilteringContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalAttributeValueRequestFiltering(options, model);
}


void scenarioDefaultRegionObjectRouting(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.ddmFom.empty(),
          "Default-region object routing requires an adapter-supplied dimensional FOM");

  Session publisher(options, model, "owner");
  Session regional(options, model, "member");
  Session mixed(options, model, "passive");
  auto const federation = federationName(options, "default-region-object-routing");
  connectAndJoin(publisher, regional, options, federation, options.ddmFom);
  mixed.connect();
  mixed.join(
      options.memberFederateName + L"-default-region-mixed",
      options.federateType,
      federation);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const regionalHandles = ddmHandles(regional, options);
  auto const mixedHandles = ddmHandles(mixed, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "default-region object routing");

  rti::AttributeHandleSet const publisherAttributes{publisherHandles.attribute};
  rti::AttributeHandleSet const regionalAttributes{regionalHandles.attribute};
  rti::AttributeHandleSet const mixedAttributes{mixedHandles.attribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);

  auto const sourceRegion = createDdmRegion(
      publisher,
      publisherHandles,
      1UL,
      5UL,
      1UL,
      5UL);
  auto const regionalRegion = createDdmRegion(
      regional,
      regionalHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const mixedRegion = createDdmRegion(
      mixed,
      mixedHandles,
      7UL,
      9UL,
      7UL,
      9UL);

  rti::AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      publisherAttributes,
      rti::RegionHandleSet{sourceRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const regionalPair{{
      regionalAttributes,
      rti::RegionHandleSet{regionalRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const mixedPair{{
      mixedAttributes,
      rti::RegionHandleSet{mixedRegion},
  }};

  regional.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  mixed.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      regional.rtiAmbassador().getConveyRegionDesignatorSetsSwitch() &&
          mixed.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "default-region receivers did not enable region designator callbacks");
  regional.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      regionalHandles.objectClass,
      regionalPair,
      true,
      L"");
  mixed.rtiAmbassador().subscribeObjectClassAttributes(
      mixedHandles.objectClass,
      mixedAttributes,
      true);
  mixed.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      mixedHandles.objectClass,
      mixedPair,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
      publisherHandles.objectClass,
      sourcePair);
  require(object.isValid(),
          "default-region object registration returned an invalid handle");
  waitFor(
      regional,
      [&] { return regional.recorder().hasDiscovery(object); },
      options,
      "default-region overlapping object discovery");
  for (int pass = 0; pass != 8; ++pass) {
    mixed.pump();
  }
  require(
      !mixed.recorder().hasDiscovery(object),
      "disjoint explicit subscription used the retained ordinary default region");

  // Removing the explicit declaration exposes the retained ordinary
  // declaration, which uses the derived default source region again.
  mixed.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      mixedHandles.objectClass,
      mixedPair);
  mixed.rtiAmbassador().subscribeObjectClassAttributes(
      mixedHandles.objectClass,
      mixedAttributes,
      true);
  waitFor(
      mixed,
      [&] { return mixed.recorder().hasDiscovery(object); },
      options,
      "default-region ordinary object discovery");
  require(
      mixed.recorder().discoveryCount() == 1U,
      "default-region ordinary discovery returned an unexpected callback count");

  // Restore the disjoint explicit declaration, then remove the publisher's
  // only explicit association. Both regional subscriptions should now match
  // the invisible default source region.
  mixed.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      mixedHandles.objectClass,
      mixedPair,
      true,
      L"");
  publisher.rtiAmbassador().unassociateRegionsForUpdates(object, sourcePair);
  regional.recorder().clearReflection();
  mixed.recorder().clearReflection();

  std::vector<std::uint8_t> defaultValue{0xD0U, 0x01U};
  std::vector<std::uint8_t> defaultTag{0x44U, 0x31U};
  rti::AttributeHandleValueMap defaultValues;
  defaultValues.emplace(
      publisherHandles.attribute,
      rti::VariableLengthData(defaultValue.data(), defaultValue.size()));
  rti::VariableLengthData defaultUserTag(defaultTag.data(), defaultTag.size());
  publisher.rtiAmbassador().updateAttributeValues(
      object,
      defaultValues,
      defaultUserTag);
  waitFor(
      regional,
      mixed,
      [&] {
        return regional.recorder().reflectionCount() >= 1U &&
            mixed.recorder().reflectionCount() >= 1U;
      },
      options,
      "default-region ordinary attribute reflection");

  auto const regionalDefault = regional.recorder().reflection();
  auto const mixedDefault = mixed.recorder().reflection();
  require(
      regionalDefault.object == object &&
          regionalDefault.values.size() == 1U &&
          regionalDefault.values.count(regionalHandles.attribute) == 1U &&
          copyBytes(regionalDefault.values.at(regionalHandles.attribute)) == defaultValue &&
          regionalDefault.tag == defaultTag &&
          regionalDefault.producer == publisher.federateHandle() &&
          regionalDefault.transportation.isValid() &&
          regionalDefault.regions.has_value() &&
          regionalDefault.regions->empty(),
      "regional default-source reflection did not preserve value metadata");
  require(
      mixedDefault.object == object &&
          mixedDefault.values.size() == 1U &&
          mixedDefault.values.count(mixedHandles.attribute) == 1U &&
          copyBytes(mixedDefault.values.at(mixedHandles.attribute)) == defaultValue &&
          mixedDefault.tag == defaultTag &&
          mixedDefault.producer == publisher.federateHandle() &&
          mixedDefault.transportation.isValid() &&
          mixedDefault.regions.has_value() &&
          mixedDefault.regions->empty(),
      "mixed default-source reflection did not preserve value metadata");

  // An explicit association replaces the default source for this attribute.
  // Only the overlapping regional subscriber should receive this update.
  publisher.rtiAmbassador().associateRegionsForUpdates(object, sourcePair);
  std::vector<std::uint8_t> explicitValue{0xE0U, 0x02U};
  std::vector<std::uint8_t> explicitTag{0x44U, 0x32U};
  rti::AttributeHandleValueMap explicitValues;
  explicitValues.emplace(
      publisherHandles.attribute,
      rti::VariableLengthData(explicitValue.data(), explicitValue.size()));
  rti::VariableLengthData explicitUserTag(explicitTag.data(), explicitTag.size());
  publisher.rtiAmbassador().updateAttributeValues(
      object,
      explicitValues,
      explicitUserTag);
  waitFor(
      regional,
      [&] { return regional.recorder().reflectionCount() >= 2U; },
      options,
      "explicit-source regional attribute reflection");
  for (int pass = 0; pass != 8; ++pass) {
    mixed.pump();
  }
  auto const regionalExplicit = regional.recorder().reflection();
  require(
      regionalExplicit.object == object &&
          regionalExplicit.values.size() == 1U &&
          regionalExplicit.values.count(regionalHandles.attribute) == 1U &&
          copyBytes(regionalExplicit.values.at(regionalHandles.attribute)) == explicitValue &&
          regionalExplicit.tag == explicitTag &&
          regionalExplicit.producer == publisher.federateHandle() &&
          regionalExplicit.transportation.isValid() &&
          regionalExplicit.regions.has_value() &&
          regionalExplicit.regions->size() == 1U &&
          regionalExplicit.regions->count(sourceRegion) == 1U,
      "explicit-source reflection did not preserve the source region designator");
  require(
      mixed.recorder().reflectionCount() == 1U,
      "disjoint explicit subscription received an explicit-source update");

  // Removing the association restores default-source fanout and the
  // supplied-empty callback projection.
  publisher.rtiAmbassador().unassociateRegionsForUpdates(object, sourcePair);
  std::vector<std::uint8_t> restoredValue{0xD0U, 0x03U};
  std::vector<std::uint8_t> restoredTag{0x44U, 0x33U};
  rti::AttributeHandleValueMap restoredValues;
  restoredValues.emplace(
      publisherHandles.attribute,
      rti::VariableLengthData(restoredValue.data(), restoredValue.size()));
  rti::VariableLengthData restoredUserTag(restoredTag.data(), restoredTag.size());
  publisher.rtiAmbassador().updateAttributeValues(
      object,
      restoredValues,
      restoredUserTag);
  waitFor(
      regional,
      mixed,
      [&] {
        return regional.recorder().reflectionCount() >= 3U &&
            mixed.recorder().reflectionCount() >= 2U;
      },
      options,
      "restored default-region attribute reflection");
  auto const regionalRestored = regional.recorder().reflection();
  auto const mixedRestored = mixed.recorder().reflection();
  require(
      regionalRestored.object == object &&
          copyBytes(regionalRestored.values.at(regionalHandles.attribute)) == restoredValue &&
          regionalRestored.tag == restoredTag &&
          regionalRestored.regions.has_value() &&
          regionalRestored.regions->empty(),
      "regional restored default-source reflection was not projected correctly");
  require(
      mixedRestored.object == object &&
          copyBytes(mixedRestored.values.at(mixedHandles.attribute)) == restoredValue &&
          mixedRestored.tag == restoredTag &&
          mixedRestored.regions.has_value() &&
          mixedRestored.regions->empty(),
      "mixed restored default-source reflection was not projected correctly");

  mixed.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      mixedHandles.objectClass,
      mixedPair);
  mixed.rtiAmbassador().unsubscribeObjectClassAttributes(
      mixedHandles.objectClass,
      mixedAttributes);
  regional.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      regionalHandles.objectClass,
      regionalPair);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  publisher.rtiAmbassador().deleteRegion(sourceRegion);
  regional.rtiAmbassador().deleteRegion(regionalRegion);
  mixed.rtiAmbassador().deleteRegion(mixedRegion);

  publisher.resign(rti::DELETE_OBJECTS);
  regional.resign(rti::NO_ACTION);
  mixed.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
  regional.disconnect();
  mixed.disconnect();
}

void scenarioDefaultRegionObjectRoutingContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDefaultRegionObjectRouting(options, model);
}


void scenarioDefaultRegionRegistrationNames(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Default-region registration testing requires an adapter-supplied dimensional FOM");

  Session publisher(options, model, "default-region-registration-publisher");
  Session observer(options, model, "default-region-registration-observer");
  auto const federation = federationName(options, "default-region-registration-names");
  connectAndJoin(publisher, observer, options, federation, options.ddmFom);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const observerHandles = ddmHandles(observer, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "default-region registration");
  require(
      publisherHandles.objectClass == observerHandles.objectClass &&
          publisherHandles.attribute == observerHandles.attribute,
      "default-region registration members resolved different object handles");

  rti::AttributeHandleSet const publisherAttributes{publisherHandles.attribute};
  rti::AttributeHandleSet const observerAttributes{observerHandles.attribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);

  auto const observerRegion = createDdmRegion(
      observer,
      observerHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  observer.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      observer.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "default-region registration observer did not enable region designator callbacks");
  rti::AttributeHandleSetRegionHandleSetPairVector const observerPair{{
      observerAttributes,
      rti::RegionHandleSet{observerRegion},
  }};
  observer.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      observerHandles.objectClass,
      observerPair,
      true,
      L"");

  // The standard regional registration surface admits both the explicit
  // empty-region form and an empty pair collection. Both select the derived
  // default source region for the available class attributes, as does the
  // ordinary registration overload.
  rti::AttributeHandleSetRegionHandleSetPairVector const emptyRegionPair{{
      publisherAttributes,
      rti::RegionHandleSet{},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const emptyPairCollection;
  auto const first = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
      publisherHandles.objectClass,
      emptyRegionPair);
  auto const second = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
      publisherHandles.objectClass,
      emptyPairCollection);
  auto const third = publisher.rtiAmbassador().registerObjectInstance(
      publisherHandles.objectClass);
  std::vector<rti::ObjectInstanceHandle> const objects{first, second, third};
  for (auto const& object : objects) {
    require(
        object.isValid(),
        "default-region registration returned an invalid object handle");
  }
  require(
      first != second && first != third && second != third,
      "default-region registration reused an object handle");

  std::vector<std::wstring> names;
  names.reserve(objects.size());
  for (auto const& object : objects) {
    auto const name = publisher.rtiAmbassador().getObjectInstanceName(object);
    require(
        !name.empty(),
        "default-region registration returned an empty generated object name");
    require(
        publisher.rtiAmbassador().getObjectInstanceHandle(name) == object,
        "default-region generated object name did not round-trip to its handle");
    require(
        std::find(names.begin(), names.end(), name) == names.end(),
        "default-region registration reused a generated object name");
    names.push_back(name);
  }

  waitFor(
      observer,
      [&] {
        return observer.recorder().discoveryCount() == objects.size();
      },
      options,
      "default-region registration discovery");
  for (std::size_t index = 0U; index != objects.size(); ++index) {
    auto const object = objects.at(index);
    require(
        observer.recorder().hasDiscovery(object),
        "default-region registration omitted an object discovery callback");
    require(
        observer.rtiAmbassador().getKnownObjectClassHandle(object) ==
            observerHandles.objectClass,
        "default-region discovery returned the wrong known object class");
    require(
        observer.rtiAmbassador().getObjectInstanceName(object) == names.at(index) &&
            observer.rtiAmbassador().getObjectInstanceHandle(names.at(index)) == object,
        "default-region discovery did not preserve generated object identity");
  }

  std::vector<std::vector<std::uint8_t>> const values{
      {0xA1U, 0x01U},
      {0xA1U, 0x02U},
      {0xA1U, 0x03U},
  };
  std::vector<std::vector<std::uint8_t>> const tags{
      {0xB1U, 0x01U},
      {0xB1U, 0x02U},
      {0xB1U, 0x03U},
  };
  for (std::size_t index = 0U; index != objects.size(); ++index) {
    rti::AttributeHandleValueMap update;
    update.emplace(
        publisherHandles.attribute,
        rti::VariableLengthData(values.at(index).data(), values.at(index).size()));
    rti::VariableLengthData tag(tags.at(index).data(), tags.at(index).size());
    publisher.rtiAmbassador().updateAttributeValues(objects.at(index), update, tag);
  }
  waitFor(
      observer,
      [&] {
        return observer.recorder().reflectionCount() == objects.size();
      },
      options,
      "default-region registration reflection");

  auto const reflections = observer.recorder().reflections();
  require(
      reflections.size() == objects.size(),
      "default-region registration returned an unexpected reflection count");
  for (std::size_t index = 0U; index != objects.size(); ++index) {
    auto const found = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](auto const& reflection) { return reflection.object == objects.at(index); });
    require(
        found != reflections.end(),
        "default-region registration omitted a reflected object update");
    require(
        found->values.size() == 1U &&
            found->values.count(observerHandles.attribute) == 1U &&
            copyBytes(found->values.at(observerHandles.attribute)) == values.at(index) &&
            found->tag == tags.at(index) &&
            found->transportation.isValid() &&
            found->producer == publisher.federateHandle() &&
            found->regions.has_value() &&
            found->regions->empty(),
        "default-region reflection did not preserve value metadata or empty region designator");
  }

  observer.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      observerHandles.objectClass,
      observerPair);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  observer.rtiAmbassador().deleteRegion(observerRegion);

  publisher.resign(rti::DELETE_OBJECTS);
  observer.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
  observer.disconnect();
}

void scenarioDefaultRegionRegistrationNamesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDefaultRegionRegistrationNames(options, model);
}


void scenarioPassiveRegionalSubscription(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.ddmFom.empty(),
          "Passive regional subscription requires an adapter-supplied dimensional FOM");

  Session publisher(options, model, "owner");
  Session subscriber(options, model, "member");
  auto const federation = federationName(options, "passive-regional-subscription");
  connectAndJoin(publisher, subscriber, options, federation, options.ddmFom);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const subscriberHandles = ddmHandles(subscriber, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "passive regional subscription");

  rti::AttributeHandleSet const publisherAttributes{publisherHandles.attribute};
  rti::AttributeHandleSet const subscriberAttributes{subscriberHandles.attribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);

  auto const sourceRegion = createDdmRegion(
      publisher,
      publisherHandles,
      1UL,
      5UL,
      1UL,
      5UL);
  auto const subscriberRegion = createDdmRegion(
      subscriber,
      subscriberHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      publisherAttributes,
      rti::RegionHandleSet{sourceRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const subscriberPair{{
      subscriberAttributes,
      rti::RegionHandleSet{subscriberRegion},
  }};

  subscriber.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      subscriber.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "passive regional subscriber did not enable region designator callbacks");
  subscriber.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      subscriberPair,
      false,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
      publisherHandles.objectClass,
      sourcePair);
  require(object.isValid(),
          "passive regional registration returned an invalid object handle");
  for (int pass = 0; pass != 8; ++pass) {
    subscriber.pump();
  }
  require(
      !subscriber.recorder().hasDiscovery(object) &&
          subscriber.recorder().reflectionCount() == 0U,
      "passive regional subscription arranged discovery or reflection");
  requireException(
      [&] {
        static_cast<void>(
            subscriber.rtiAmbassador().getKnownObjectClassHandle(object));
      },
      L"ObjectInstanceNotKnown",
      "known-object lookup for a passively subscribed regional object");

  std::vector<std::uint8_t> passiveValue{0x50U, 0x60U};
  rti::AttributeHandleValueMap passiveValues;
  passiveValues.emplace(
      publisherHandles.attribute,
      rti::VariableLengthData(passiveValue.data(), passiveValue.size()));
  publisher.rtiAmbassador().updateAttributeValues(
      object,
      passiveValues,
      rti::VariableLengthData{});
  for (int pass = 0; pass != 8; ++pass) {
    subscriber.pump();
  }
  require(
      subscriber.recorder().reflectionCount() == 0U,
      "passive regional subscription received an attribute update");

  // Replacing the same regional declaration with an active one makes the
  // already-registered, in-scope object discoverable.
  subscriber.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      subscriberPair,
      true,
      L"");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "active regional discovery after passive declaration");
  auto const discovery = subscriber.recorder().discovery();
  require(
      discovery.object == object &&
          discovery.objectClass == subscriberHandles.objectClass &&
          discovery.producer == publisher.federateHandle(),
      "active regional discovery returned the wrong object metadata");

  std::vector<std::uint8_t> activeValue{0x70U, 0x80U};
  std::vector<std::uint8_t> activeTag{0x50U, 0x52U};
  rti::AttributeHandleValueMap activeValues;
  activeValues.emplace(
      publisherHandles.attribute,
      rti::VariableLengthData(activeValue.data(), activeValue.size()));
  rti::VariableLengthData activeUserTag(activeTag.data(), activeTag.size());
  publisher.rtiAmbassador().updateAttributeValues(
      object,
      activeValues,
      activeUserTag);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflectionCount() >= 1U; },
      options,
      "active regional reflection after passive declaration");
  auto const reflection = subscriber.recorder().reflection();
  require(
      reflection.object == object &&
          reflection.values.size() == 1U &&
          reflection.values.count(subscriberHandles.attribute) == 1U &&
          copyBytes(reflection.values.at(subscriberHandles.attribute)) == activeValue &&
          reflection.tag == activeTag &&
          reflection.producer == publisher.federateHandle() &&
          reflection.transportation.isValid() &&
          reflection.regions.has_value() &&
          reflection.regions->size() == 1U &&
          reflection.regions->count(sourceRegion) == 1U,
      "active regional reflection did not preserve source-region metadata");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      subscriberPair);
  publisher.rtiAmbassador().unassociateRegionsForUpdates(object, sourcePair);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  publisher.rtiAmbassador().deleteRegion(sourceRegion);
  subscriber.rtiAmbassador().deleteRegion(subscriberRegion);

  publisher.resign(rti::DELETE_OBJECTS);
  subscriber.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
  subscriber.disconnect();
}

void scenarioPassiveRegionalSubscriptionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioPassiveRegionalSubscription(options, model);
}

void scenarioPassiveRegionalInteractionTransition(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Passive regional interaction transition requires an adapter-supplied dimensional FOM");

  Session publisher(options, model, "passive-interaction-publisher");
  Session active(options, model, "passive-interaction-active");
  Session passive(options, model, "passive-interaction-passive");
  auto const federation = federationName(
      options,
      "passive-regional-interaction-transition");
  connectAndJoin(publisher, active, options, federation, options.ddmFom);
  passive.connect();
  passive.join(
      options.memberFederateName + L"-passive-regional-interaction",
      options.federateType,
      federation);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const activeHandles = ddmHandles(active, options);
  auto const passiveHandles = ddmHandles(passive, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "passive regional interaction transition publisher");
  verifyDdmClassDimensions(
      active,
      options,
      activeHandles,
      "passive regional interaction transition active member");
  verifyDdmClassDimensions(
      passive,
      options,
      passiveHandles,
      "passive regional interaction transition passive member");
  require(
      publisherHandles.interactionClass == activeHandles.interactionClass &&
          publisherHandles.interactionClass == passiveHandles.interactionClass &&
          publisherHandles.parameter == activeHandles.parameter &&
          publisherHandles.parameter == passiveHandles.parameter,
      "passive regional interaction members resolved different declaration handles");

  publisher.rtiAmbassador().publishInteractionClass(
      publisherHandles.interactionClass);
  auto const sourceRegion = createDdmRegion(
      publisher,
      publisherHandles,
      1UL,
      5UL,
      1UL,
      5UL);
  auto const activeRegion = createDdmRegion(
      active,
      activeHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const passiveRegion = createDdmRegion(
      passive,
      passiveHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const sourceRegionSet = rti::RegionHandleSet{sourceRegion};
  auto const activeRegionSet = rti::RegionHandleSet{activeRegion};
  auto const passiveRegionSet = rti::RegionHandleSet{passiveRegion};

  active.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  passive.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      active.rtiAmbassador().getConveyRegionDesignatorSetsSwitch() &&
          passive.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "passive regional interaction receivers did not enable region designator callbacks");

  active.rtiAmbassador().subscribeInteractionClassWithRegions(
      activeHandles.interactionClass,
      activeRegionSet,
      true);
  // The second member starts with an active regional declaration so that the
  // transition to passive can be checked without changing its region pair.
  passive.rtiAmbassador().subscribeInteractionClassWithRegions(
      passiveHandles.interactionClass,
      passiveRegionSet,
      true);

  std::vector<std::uint8_t> const valueBytes{0x2aU, 0x15U};
  std::vector<std::uint8_t> const firstTag{0x70U, 0x31U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherHandles.parameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));

  auto send = [&](std::vector<std::uint8_t> const& tagBytes) {
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().sendInteractionWithRegions(
        publisherHandles.interactionClass,
        parameters,
        sourceRegionSet,
        tag);
  };
  auto pumpReceivers = [&] {
    for (int pass = 0; pass != 8; ++pass) {
      active.pump();
      passive.pump();
    }
  };
  auto requireInteraction = [&](
      Session& receiver,
      DdmHandles const& receiverHandles,
      std::vector<std::uint8_t> const& expectedTag,
      std::string const& description) {
    auto const records = receiver.recorder().interactions();
    require(
        records.size() == 1U,
        description + " returned an unexpected callback count");
    auto const& record = records.front();
    require(
        record.interaction == receiverHandles.interactionClass &&
            record.parameters.size() == 1U &&
            record.parameters.count(receiverHandles.parameter) == 1U &&
            copyBytes(record.parameters.at(receiverHandles.parameter)) == valueBytes &&
            record.tag == expectedTag &&
            record.producer == publisher.federateHandle() &&
            record.transportation.isValid(),
        description + " changed interaction delivery metadata");
    require(
        record.regions.has_value() &&
            record.regions->size() == 1U &&
            record.regions->count(sourceRegion) == 1U,
        description + " did not convey the source-region designator set");
  };

  // Both overlapping regional declarations are active initially.
  send(firstTag);
  waitForSessions(
      std::vector<Session*>{&active, &passive},
      [&] {
        return active.recorder().interactions().size() >= 1U &&
            passive.recorder().interactions().size() >= 1U;
      },
      options,
      "passive regional interaction initial delivery");
  requireInteraction(
      active,
      activeHandles,
      firstTag,
      "active regional interaction initial delivery");
  requireInteraction(
      passive,
      passiveHandles,
      firstTag,
      "passive regional interaction initial delivery");

  active.recorder().clearInteraction();
  passive.recorder().clearInteraction();
  // A passive declaration remains a delivery target while another active
  // subscriber keeps the interaction relevant for the federation.
  passive.rtiAmbassador().subscribeInteractionClassWithRegions(
      passiveHandles.interactionClass,
      passiveRegionSet,
      false);
  auto const retainedTag = std::vector<std::uint8_t>{0x70U, 0x32U};
  send(retainedTag);
  waitForSessions(
      std::vector<Session*>{&active, &passive},
      [&] {
        return active.recorder().interactions().size() >= 1U &&
            passive.recorder().interactions().size() >= 1U;
      },
      options,
      "passive regional interaction retained delivery");
  requireInteraction(
      active,
      activeHandles,
      retainedTag,
      "active regional interaction retained delivery");
  requireInteraction(
      passive,
      passiveHandles,
      retainedTag,
      "passive regional interaction retained delivery");

  active.recorder().clearInteraction();
  passive.recorder().clearInteraction();
  // Once the last active route is removed, the stored passive pair no longer
  // establishes relevance and neither receiver should see a new send.
  active.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      activeHandles.interactionClass,
      activeRegionSet);
  send(std::vector<std::uint8_t>{0x70U, 0x33U});
  pumpReceivers();
  require(
      active.recorder().interactions().empty() &&
          passive.recorder().interactions().empty(),
      "passive regional interaction delivered without an active route");

  // An empty region set is a no-op; it must not convert the retained passive
  // declaration back into an active subscription.
  passive.rtiAmbassador().subscribeInteractionClassWithRegions(
      passiveHandles.interactionClass,
      rti::RegionHandleSet{},
      true);
  send(std::vector<std::uint8_t>{0x70U, 0x34U});
  pumpReceivers();
  require(
      passive.recorder().interactions().empty(),
      "an empty regional interaction subscription restored active relevance");

  passive.rtiAmbassador().subscribeInteractionClassWithRegions(
      passiveHandles.interactionClass,
      passiveRegionSet,
      true);
  auto const restoredTag = std::vector<std::uint8_t>{0x70U, 0x35U};
  send(restoredTag);
  waitFor(
      passive,
      [&] { return passive.recorder().interactions().size() >= 1U; },
      options,
      "passive regional interaction restored delivery");
  requireInteraction(
      passive,
      passiveHandles,
      restoredTag,
      "passive regional interaction restored delivery");
  require(
      active.recorder().interactions().empty(),
      "unsubscribed active regional interaction receiver received a message");

  passive.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      passiveHandles.interactionClass,
      passiveRegionSet);
  publisher.rtiAmbassador().unpublishInteractionClass(
      publisherHandles.interactionClass);
  publisher.rtiAmbassador().deleteRegion(sourceRegion);
  active.rtiAmbassador().deleteRegion(activeRegion);
  passive.rtiAmbassador().deleteRegion(passiveRegion);
  passive.resign(rti::NO_ACTION);
  active.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  passive.disconnect();
  active.disconnect();
  publisher.disconnect();
}

void scenarioPassiveRegionalInteractionTransitionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioPassiveRegionalInteractionTransition(options, model);
}

} // namespace hla_rti_cpp_tck::detail
