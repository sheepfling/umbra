#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioRegisterObjectInstanceWithRegionsInvalidRegion(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "invalid-region-registration-owner");
  Session subscriber(options, model, "invalid-region-registration-subscriber");
  auto const federation = federationName(options, "register-regions-invalid-region");
  connectAndJoin(owner, subscriber, options, federation, options.multiAttributeFom);

  auto& ambassador = owner.rtiAmbassador();
  auto const objectClass =
      ambassador.getObjectClassHandle(options.multiAttributeObjectClassName);
  auto const attribute =
      ambassador.getAttributeHandle(objectClass, options.multiAttributeFirstName);
  auto const dimension =
      ambassador.getDimensionHandle(options.ddmDimensionNames.at(0));
  require(objectClass.isValid() && attribute.isValid() && dimension.isValid(),
          "invalid-region registration received an invalid FOM handle");
  ambassador.publishObjectClassAttributes(
      objectClass, rti::AttributeHandleSet{attribute});

  auto const staleRegion = ambassador.createRegion(rti::DimensionHandleSet{dimension});
  require(staleRegion.isValid(),
          "invalid-region registration could not create its region fixture");
  ambassador.deleteRegion(staleRegion);

  rti::AttributeHandleSetRegionHandleSetPairVector const invalidRegionPair{{
      rti::AttributeHandleSet{attribute},
      rti::RegionHandleSet{staleRegion},
  }};
  requireException(
      [&] {
        static_cast<void>(
            ambassador.registerObjectInstanceWithRegions(objectClass, invalidRegionPair));
      },
      L"InvalidRegion",
      "registering an object instance with a deleted region designator");

  subscriber.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  ambassador.destroyFederationExecution(federation);
  subscriber.disconnect();
  owner.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
