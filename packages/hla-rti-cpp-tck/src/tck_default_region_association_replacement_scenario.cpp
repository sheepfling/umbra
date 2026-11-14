#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

struct DefaultAssociationHandles final {
  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::DimensionHandle firstDimension;
  rti::DimensionHandle secondDimension;
};

DefaultAssociationHandles defaultAssociationHandles(
    Session& session,
    Options const& options) {
  auto& ambassador = session.rtiAmbassador();
  DefaultAssociationHandles result;
  result.objectClass = ambassador.getObjectClassHandle(options.objectClassName);
  result.attribute =
      ambassador.getAttributeHandle(result.objectClass, options.attributeName);
  require(options.ddmDimensionNames.size() >= 2U,
          "default-association replacement requires two adapter DDM dimensions");
  result.firstDimension =
      ambassador.getDimensionHandle(options.ddmDimensionNames.at(0));
  result.secondDimension =
      ambassador.getDimensionHandle(options.ddmDimensionNames.at(1));
  require(result.objectClass.isValid() && result.attribute.isValid() &&
              result.firstDimension.isValid() && result.secondDimension.isValid(),
          "default-association replacement received invalid FOM handles");
  return result;
}

rti::RegionHandle createDefaultAssociationRegion(
    Session& session,
    DefaultAssociationHandles const& handles,
    unsigned long lower,
    unsigned long upper) {
  auto& ambassador = session.rtiAmbassador();
  auto const region = ambassador.createRegion(
      rti::DimensionHandleSet{handles.firstDimension, handles.secondDimension});
  require(region.isValid(), "default-association replacement created an invalid region");
  ambassador.setRangeBounds(
      region, handles.firstDimension, rti::RangeBounds(lower, upper));
  ambassador.setRangeBounds(
      region, handles.secondDimension, rti::RangeBounds(lower, upper));
  ambassador.commitRegionModifications(rti::RegionHandleSet{region});
  return region;
}

}  // namespace

void scenarioAssociateRegionsForUpdatesReplacesDefaultRegion(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "default-association-owner");
  Session overlap(options, model, "default-association-overlap");
  Session disjoint(options, model, "default-association-disjoint");
  auto const federation =
      federationName(options, "associate-regions-replaces-default-association");
  connectAndJoin(owner, overlap, options, federation, options.ddmFom);
  disjoint.connect();
  disjoint.join(
      options.memberFederateName + L"-default-association-disjoint",
      options.federateType,
      federation);

  auto const ownerHandles = defaultAssociationHandles(owner, options);
  auto const overlapHandles = defaultAssociationHandles(overlap, options);
  auto const disjointHandles = defaultAssociationHandles(disjoint, options);
  rti::AttributeHandleSet const ownerAttributes{ownerHandles.attribute};
  rti::AttributeHandleSet const overlapAttributes{overlapHandles.attribute};
  rti::AttributeHandleSet const disjointAttributes{disjointHandles.attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerHandles.objectClass, ownerAttributes);

  auto const sourceRegion =
      createDefaultAssociationRegion(owner, ownerHandles, 1UL, 5UL);
  auto const overlapRegion =
      createDefaultAssociationRegion(overlap, overlapHandles, 2UL, 4UL);
  auto const disjointRegion =
      createDefaultAssociationRegion(disjoint, disjointHandles, 7UL, 9UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const overlapSubscription{{
      overlapAttributes,
      rti::RegionHandleSet{overlapRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const disjointSubscription{{
      disjointAttributes,
      rti::RegionHandleSet{disjointRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const explicitSource{{
      ownerAttributes,
      rti::RegionHandleSet{sourceRegion},
  }};
  overlap.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  disjoint.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  overlap.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      overlapHandles.objectClass, overlapSubscription, true, L"");
  disjoint.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      disjointHandles.objectClass, disjointSubscription, true, L"");

  auto const object =
      owner.rtiAmbassador().registerObjectInstance(ownerHandles.objectClass);
  require(object.isValid(),
          "default-association registration returned an invalid object handle");
  waitFor(
      overlap,
      [&] { return overlap.recorder().hasDiscovery(object); },
      options,
      "default-source overlap discovery");
  waitFor(
      disjoint,
      [&] { return disjoint.recorder().hasDiscovery(object); },
      options,
      "default-source disjoint discovery");

  auto update = [&](std::vector<std::uint8_t> const& bytes,
                    std::vector<std::uint8_t> const& tag) {
    rti::AttributeHandleValueMap values;
    values.emplace(
        ownerHandles.attribute,
        rti::VariableLengthData(bytes.data(), bytes.size()));
    rti::VariableLengthData userTag(tag.data(), tag.size());
    owner.rtiAmbassador().updateAttributeValues(object, values, userTag);
  };

  std::vector<std::uint8_t> const defaultValue{0xD0U, 0x01U};
  std::vector<std::uint8_t> const defaultTag{0xA1U, 0x01U};
  update(defaultValue, defaultTag);
  waitFor(
      overlap,
      [&] { return overlap.recorder().reflection().present; },
      options,
      "default-source update to overlapping regional subscriber");
  waitFor(
      disjoint,
      [&] { return disjoint.recorder().reflection().present; },
      options,
      "default-source update to disjoint regional subscriber");
  auto const overlapDefault = overlap.recorder().reflection();
  auto const disjointDefault = disjoint.recorder().reflection();
  require(overlapDefault.regions.has_value() && overlapDefault.regions->empty() &&
              disjointDefault.regions.has_value() && disjointDefault.regions->empty(),
          "default-source update conveyed an explicit source region");
  require(copyBytes(overlapDefault.values.at(overlapHandles.attribute)) == defaultValue &&
              copyBytes(disjointDefault.values.at(disjointHandles.attribute)) == defaultValue,
          "default-source update did not reach both regional subscriptions");

  overlap.recorder().clearReflection();
  disjoint.recorder().clearReflection();
  owner.rtiAmbassador().associateRegionsForUpdates(object, explicitSource);
  std::vector<std::uint8_t> const explicitValue{0xE0U, 0x02U};
  std::vector<std::uint8_t> const explicitTag{0xA1U, 0x02U};
  update(explicitValue, explicitTag);
  waitFor(
      overlap,
      [&] { return overlap.recorder().reflection().present; },
      options,
      "explicit-source update after replacing the default association");
  for (int pass = 0; pass != 8; ++pass) {
    disjoint.pump();
  }
  auto const overlapExplicit = overlap.recorder().reflection();
  require(copyBytes(overlapExplicit.values.at(overlapHandles.attribute)) == explicitValue &&
              overlapExplicit.tag == explicitTag && overlapExplicit.regions.has_value() &&
              overlapExplicit.regions->size() == 1U,
          "explicit-source update lost its value, tag, or source-region designator");
  require(!disjoint.recorder().reflection().present,
          "disjoint regional subscription still received the replaced default source");

  owner.rtiAmbassador().unassociateRegionsForUpdates(object, explicitSource);
  overlap.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      overlapHandles.objectClass, overlapSubscription);
  disjoint.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      disjointHandles.objectClass, disjointSubscription);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerHandles.objectClass, ownerAttributes);
  owner.rtiAmbassador().deleteRegion(sourceRegion);
  overlap.rtiAmbassador().deleteRegion(overlapRegion);
  disjoint.rtiAmbassador().deleteRegion(disjointRegion);
  disjoint.resign(rti::NO_ACTION);
  overlap.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  disjoint.disconnect();
  overlap.disconnect();
  owner.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
