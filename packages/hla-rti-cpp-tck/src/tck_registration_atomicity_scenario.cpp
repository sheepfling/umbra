#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioRegisterObjectInstanceWithRegionsAtomicity(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "registration-atomicity-owner");
  Session overlap(options, model, "registration-atomicity-overlap");
  Session disjoint(options, model, "registration-atomicity-disjoint");
  auto const federation = federationName(options, "regional-registration-atomicity");
  connectAndJoin(owner, overlap, options, federation, options.ddmFom);
  disjoint.connect();
  disjoint.join(
      options.memberFederateName + L"-regional-disjoint",
      options.federateType,
      federation);

  auto const ownerHandles = ddmHandles(owner, options);
  auto const overlapHandles = ddmHandles(overlap, options);
  auto const disjointHandles = ddmHandles(disjoint, options);
  verifyDdmClassDimensions(
      owner, options, ownerHandles, "regional registration atomicity");

  rti::AttributeHandleSet const ownerAttributes{ownerHandles.attribute};
  rti::AttributeHandleSet const overlapAttributes{overlapHandles.attribute};
  rti::AttributeHandleSet const disjointAttributes{disjointHandles.attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerHandles.objectClass, ownerAttributes);

  auto const sourceRegion = createDdmRegion(owner, ownerHandles, 1UL, 5UL, 1UL, 5UL);
  auto const overlapRegion =
      createDdmRegion(overlap, overlapHandles, 2UL, 6UL, 2UL, 6UL);
  auto const disjointRegion =
      createDdmRegion(disjoint, disjointHandles, 7UL, 9UL, 7UL, 9UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      ownerAttributes,
      rti::RegionHandleSet{sourceRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const overlapPair{{
      overlapAttributes,
      rti::RegionHandleSet{overlapRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const disjointPair{{
      disjointAttributes,
      rti::RegionHandleSet{disjointRegion},
  }};
  overlap.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      overlapHandles.objectClass, overlapPair, true, L"");
  disjoint.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      disjointHandles.objectClass, disjointPair, true, L"");

  auto const object = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerHandles.objectClass, sourcePair);
  require(object.isValid(), "regional atomic registration returned an invalid object handle");
  waitFor(
      overlap,
      [&] { return overlap.recorder().hasDiscovery(object); },
      options,
      "overlapping regional discovery after atomic registration");
  for (int pass = 0; pass != 8; ++pass) {
    disjoint.pump();
  }
  require(
      !disjoint.recorder().hasDiscovery(object),
      "Register Object Instance With Regions exposed the object to a disjoint "
      "subscriber before applying its update-region association");

  disjoint.disconnect();
  overlap.disconnect();
  owner.disconnect();
}

} // namespace hla_rti_cpp_tck::detail
