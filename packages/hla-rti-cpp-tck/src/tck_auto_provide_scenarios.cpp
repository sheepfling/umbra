#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioAutoProvide(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.autoProvideFom.empty(),
      "Auto Provide testing requires an adapter-supplied Auto Provide FOM");
  Session owner(options, model, "owner");
  Session requester(options, model, "requester");
  auto const federation = federationName(options, "auto-provide");
  connectAndJoin(owner, requester, options, federation, options.autoProvideFom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.autoProvideObjectClassName);
  auto const ownerFirst = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.autoProvideFirstAttributeName);
  auto const ownerSecond = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.autoProvideSecondAttributeName);
  auto const requesterFirst = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.autoProvideFirstAttributeName);
  auto const requesterSecond = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.autoProvideSecondAttributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() && ownerFirst.isValid() &&
          ownerSecond.isValid() && requesterFirst.isValid() && requesterSecond.isValid(),
      "Auto Provide handle lookup returned an invalid handle");
  require(
      owner.rtiAmbassador().getAutoProvideSwitch() &&
          requester.rtiAmbassador().getAutoProvideSwitch(),
      "Auto Provide FOM did not enable the standard Auto Provide switch");

  rti::AttributeHandleSet ownerAttributes{ownerFirst, ownerSecond};
  rti::AttributeHandleSet requesterAttributes{requesterFirst, requesterSecond};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass, requesterAttributes, true, L"");
  requester.rtiAmbassador().setAttributeScopeAdvisorySwitch(false);
  require(!requester.rtiAmbassador().getAttributeScopeAdvisorySwitch(),
          "Auto Provide baseline did not disable Attribute Scope Advisory");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "Auto Provide registration returned an invalid object handle");

  waitFor(
      owner,
      requester,
      [&] {
        return owner.recorder().providedUpdates().size() >= 1U &&
            requester.recorder().discovery().present;
      },
      options,
      "Auto Provide discovery and solicitation");
  auto const providedUpdates = owner.recorder().providedUpdates();
  require(
      providedUpdates.size() == 1U,
      "Auto Provide solicited more than one provider callback");
  auto const& provided = providedUpdates.front();
  require(
      provided.object == object && provided.attributes == ownerAttributes &&
          provided.tag.empty(),
      "Auto Provide callback returned the wrong object, attributes, or tag");
  auto const discovery = requester.recorder().discovery();
  require(
      discovery.object == object && discovery.objectClass == requesterClass,
      "Auto Provide discovery returned the wrong object or object class");

  // The standard Auto Provide solicitation must be the same when only the
  // discovering federate's Attribute Scope Advisory switch is enabled.
  requester.rtiAmbassador().setAttributeScopeAdvisorySwitch(true);
  require(requester.rtiAmbassador().getAttributeScopeAdvisorySwitch(),
          "Auto Provide comparison did not enable Attribute Scope Advisory");
  require(owner.rtiAmbassador().getAutoProvideSwitch() &&
              requester.rtiAmbassador().getAutoProvideSwitch(),
          "Attribute Scope Advisory changed the Auto Provide switch");
  auto const secondObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(secondObject.isValid() && secondObject != object,
          "Auto Provide comparison returned an invalid or duplicate object handle");
  waitFor(
      owner,
      requester,
      [&] {
        return owner.recorder().providedUpdates().size() >= 2U &&
            requester.recorder().hasDiscovery(secondObject);
      },
      options,
      "Auto Provide solicitation with Attribute Scope Advisory enabled");
  auto const secondProvidedUpdates = owner.recorder().providedUpdates();
  require(secondProvidedUpdates.size() == 2U,
          "Auto Provide with Attribute Scope Advisory enabled changed the callback count");
  auto const& secondProvided = secondProvidedUpdates.back();
  require(secondProvided.object == secondObject &&
              secondProvided.attributes == ownerAttributes &&
              secondProvided.tag.empty(),
          "Auto Provide with Attribute Scope Advisory enabled changed the callback data");
  require(requester.recorder().discoveryCount() == 2U,
          "Auto Provide comparison changed the discovery count");

  requester.rtiAmbassador().unsubscribeObjectClassAttributes(
      requesterClass, requesterAttributes);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioAutoProvideContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAutoProvide(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
