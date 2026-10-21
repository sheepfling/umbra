#include "tck_main_scenarios.hpp"

#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioResignDivestsAllInstanceAttributes(Options const& options,
                                                rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Multi-instance resignation requires an adapter-supplied FOM");
  Session owner(options, model, "resign-all-owner");
  Session survivor(options, model, "resign-all-survivor");
  auto const federation = federationName(options, "resign-divests-all-instances");
  owner.connect();
  survivor.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName + L"-resign-all-owner",
             options.federateType, federation);
  survivor.join(options.memberFederateName + L"-resign-all-survivor",
                options.federateType, federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const survivorClass = survivor.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerFirst = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  auto const ownerSecond = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.secondaryAttributeName);
  auto const survivorFirst = survivor.rtiAmbassador().getAttributeHandle(
      survivorClass, options.attributeName);
  auto const survivorSecond = survivor.rtiAmbassador().getAttributeHandle(
      survivorClass, options.secondaryAttributeName);
  require(ownerClass.isValid() && survivorClass.isValid() &&
              ownerFirst.isValid() && ownerSecond.isValid() &&
              survivorFirst.isValid() && survivorSecond.isValid() &&
              ownerFirst != ownerSecond && survivorFirst != survivorSecond,
          "Adapter FOM did not resolve two distinct resign-time attributes");
  rti::AttributeHandleSet const ownerAttributes{ownerFirst, ownerSecond};
  rti::AttributeHandleSet const survivorAttributes{
      survivorFirst, survivorSecond};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  survivor.rtiAmbassador().subscribeObjectClassAttributes(
      survivorClass, survivorAttributes, true, L"");

  std::vector<rti::ObjectInstanceHandle> const objects{
      owner.rtiAmbassador().registerObjectInstance(ownerClass),
      owner.rtiAmbassador().registerObjectInstance(ownerClass)};
  require(objects[0].isValid() && objects[1].isValid() &&
              objects[0] != objects[1],
          "Multi-instance registration did not return two distinct objects");
  waitFor(survivor,
          [&] {
            return survivor.recorder().hasDiscovery(objects[0]) &&
                survivor.recorder().hasDiscovery(objects[1]);
          }, options, "two-object resignation discovery");
  for (auto const& object : objects) {
    require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
                owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerSecond) &&
                !survivor.rtiAmbassador().isAttributeOwnedByFederate(
                    object, survivorFirst) &&
                !survivor.rtiAmbassador().isAttributeOwnedByFederate(
                    object, survivorSecond),
            "Registrant did not own both attributes on each instance before resignation");
  }

  survivor.recorder().clearRemovals();
  owner.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  for (auto const& object : objects) {
    require(survivor.rtiAmbassador().getKnownObjectClassHandle(object) ==
                    survivorClass &&
                !survivor.rtiAmbassador().isAttributeOwnedByFederate(
                    object, survivorFirst) &&
                !survivor.rtiAmbassador().isAttributeOwnedByFederate(
                    object, survivorSecond),
            "Unconditional resignation did not retain and divest each object");
    survivor.recorder().clearOwnershipRecords();
    survivor.rtiAmbassador().queryAttributeOwnership(object, survivorAttributes);
    waitFor(survivor,
            [&] { return survivor.recorder().ownershipNotOwned().has_value(); },
            options, "post-resignation unowned attribute report");
    auto const unowned = survivor.recorder().ownershipNotOwned();
    require(unowned->object == object &&
                unowned->attributes == survivorAttributes &&
                !survivor.recorder().ownershipInformation().has_value() &&
                !survivor.recorder().ownershipOwnedByRti().has_value(),
            "Post-resignation ownership query did not report both attributes unowned");
  }
  require(survivor.recorder().removals().empty(),
          "Unconditional divestiture removed a registered object");

  survivor.resign(rti::NO_ACTION);
  survivor.rtiAmbassador().destroyFederationExecution(federation);
  survivor.disconnect();
  owner.disconnect();
}

void scenarioResignDivestsAllInstanceAttributesContract(
    Options const& options, rti::CallbackModel model) {
  scenarioResignDivestsAllInstanceAttributes(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
