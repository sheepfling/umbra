#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioOwnershipAfterFomMerge(Options const& options,
                                    rti::CallbackModel model) {
  require(!options.fom.empty() && !options.ownershipExtensionFom.empty(),
          "Post-join ownership requires adapter-supplied base and extension FOMs");
  require(model == rti::HLA_EVOKED,
          "Post-join ownership notification timing requires evoked callbacks");

  Session owner(options, model, "fom-merge-owner");
  Session joiner(options, model, "fom-merge-joiner");
  auto const federation = federationName(options, "ownership-after-fom-merge");
  owner.connect();
  joiner.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName + L"-fom-merge-owner",
             options.federateType, federation);
  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const oldAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  require(ownerClass.isValid() && oldAttribute.isValid(),
          "Adapter base FOM did not resolve the original class and attribute");
  rti::AttributeHandleSet const oldAttributes{oldAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, oldAttributes);
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid() &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, oldAttribute),
          "Registrant did not own the original attribute before FOM merge");

  joiner.join(options.memberFederateName + L"-fom-merge-joiner",
              options.federateType, federation,
              std::vector<std::wstring>{options.ownershipExtensionFom.wstring()});
  auto const joinerClass = joiner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const joinerOldAttribute = joiner.rtiAmbassador().getAttributeHandle(
      joinerClass, options.attributeName);
  auto const addedAttribute = joiner.rtiAmbassador().getAttributeHandle(
      joinerClass, options.ownershipAddedAttributeName);
  require(joinerClass.isValid() && joinerOldAttribute.isValid() &&
              addedAttribute.isValid() && addedAttribute != joinerOldAttribute,
          "Adapter extension FOM did not add the expected attribute to the class");
  rti::AttributeHandleSet const subscribed{
      joinerOldAttribute, addedAttribute};
  rti::AttributeHandleSet const addedOnly{addedAttribute};
  joiner.rtiAmbassador().subscribeObjectClassAttributes(
      joinerClass, subscribed, true, L"");
  waitFor(joiner,
          [&] { return joiner.recorder().hasDiscovery(object); },
          options, "preexisting object discovery after FOM merge");
  require(joiner.rtiAmbassador().getKnownObjectClassHandle(object) == joinerClass,
          "FOM merge discoverer learned the wrong object class");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, oldAttribute) &&
              !owner.rtiAmbassador().isAttributeOwnedByFederate(
                  object, addedAttribute) &&
              !joiner.rtiAmbassador().isAttributeOwnedByFederate(
                  object, joinerOldAttribute) &&
              !joiner.rtiAmbassador().isAttributeOwnedByFederate(
                  object, addedAttribute),
          "New FOM attribute inherited ownership of a preexisting instance");

  joiner.rtiAmbassador().publishObjectClassAttributes(joinerClass, addedOnly);
  require(!owner.rtiAmbassador().isAttributeOwnedByFederate(object, addedAttribute) &&
              !joiner.rtiAmbassador().isAttributeOwnedByFederate(
                  object, addedAttribute),
          "Publication alone transferred ownership of the added attribute");

  std::vector<std::uint8_t> const tagBytes{0xA9U, 0x25U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  joiner.recorder().clearOwnershipRecords();
  joiner.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object, addedOnly, tag);
  require(!joiner.rtiAmbassador().isAttributeOwnedByFederate(
              object, addedAttribute) &&
              !joiner.recorder().ownershipAcquisition().has_value(),
          "Added attribute transferred before acquisition notification");
  waitFor(joiner,
          [&] { return joiner.recorder().ownershipAcquisition().has_value(); },
          options, "post-merge ownership acquisition notification");
  auto const notification = joiner.recorder().ownershipAcquisition();
  require(notification->object == object &&
              notification->attributes == addedOnly &&
              notification->tag == tagBytes &&
              !joiner.recorder().ownershipUnavailable().has_value(),
          "Post-merge acquisition returned the wrong standard callback");
  require(joiner.rtiAmbassador().isAttributeOwnedByFederate(
              object, addedAttribute) &&
              !owner.rtiAmbassador().isAttributeOwnedByFederate(
                  object, addedAttribute),
          "Post-merge acquisition notification did not establish ownership");

  joiner.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  joiner.disconnect();
  owner.disconnect();
}

void scenarioOwnershipAfterFomMergeContract(Options const& options,
                                            rti::CallbackModel model) {
  scenarioOwnershipAfterFomMerge(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
