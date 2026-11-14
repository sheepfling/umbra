#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioNegotiatedDivestitureIfWantedCompletion(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "alternate negotiated-divestiture completion requires an adapter-supplied FOM");

  Session owner(options, model, "negotiated-if-wanted-owner");
  Session acquirer(options, model, "negotiated-if-wanted-acquirer");
  auto const federation = federationName(
      options,
      "negotiated-divestiture-if-wanted-completion");
  connectAndJoin(owner, acquirer, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const acquirerClass = acquirer.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const acquirerAttribute = acquirer.rtiAmbassador().getAttributeHandle(
      acquirerClass,
      options.attributeName);
  require(
      ownerClass.isValid() && acquirerClass.isValid() &&
          ownerAttribute.isValid() && acquirerAttribute.isValid(),
      "alternate negotiated-divestiture lookup returned an invalid handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const acquirerAttributes{acquirerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  acquirer.rtiAmbassador().publishObjectClassAttributes(
      acquirerClass,
      acquirerAttributes);
  acquirer.rtiAmbassador().subscribeObjectClassAttributes(
      acquirerClass,
      acquirerAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "alternate negotiated-divestiture registration failed");
  waitFor(
      acquirer,
      [&] { return acquirer.recorder().hasDiscovery(object); },
      options,
      "alternate negotiated-divestiture object discovery");

  std::vector<std::uint8_t> const offerTagBytes{0xD7U, 0x41U};
  std::vector<std::uint8_t> const acquisitionTagBytes{0xA7U, 0x41U};
  std::vector<std::uint8_t> const divestitureTagBytes{0xE7U, 0x41U};
  rti::VariableLengthData const offerTag(
      offerTagBytes.data(),
      offerTagBytes.size());
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(),
      divestitureTagBytes.size());

  owner.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
      object,
      ownerAttributes,
      offerTag);
  waitFor(
      acquirer,
      [&] { return acquirer.recorder().ownershipAssumption().has_value(); },
      options,
      "negotiated divestiture ownership-assumption request");
  auto const assumption = acquirer.recorder().ownershipAssumption();
  require(
      assumption->object == object && assumption->attributes == acquirerAttributes,
      "negotiated divestiture did not identify the eligible acquirer");

  acquirer.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      acquirerAttributes,
      acquisitionTag);
  waitFor(
      owner,
      [&] { return owner.recorder().divestitureConfirmation().has_value(); },
      options,
      "negotiated divestiture confirmation request before alternate completion");
  auto const confirmation = owner.recorder().divestitureConfirmation();
  require(
      confirmation->object == object &&
          confirmation->attributes == ownerAttributes &&
          confirmation->tag == acquisitionTagBytes &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !acquirer.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              acquirerAttribute),
      "negotiated divestiture was not pending before the alternate completion service");

  rti::AttributeHandleSet divestedAttributes;
  owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object,
      ownerAttributes,
      divestitureTag,
      divestedAttributes);
  require(
      divestedAttributes == ownerAttributes,
      "If Wanted did not complete the pending negotiated divestiture");
  waitFor(
      acquirer,
      [&] { return acquirer.recorder().ownershipAcquisition().has_value(); },
      options,
      "alternate negotiated-divestiture acquisition notification");
  auto const acquisition = acquirer.recorder().ownershipAcquisition();
  require(
      acquisition->object == object &&
          acquisition->attributes == acquirerAttributes &&
          acquisition->tag == divestitureTagBytes &&
          !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          acquirer.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              acquirerAttribute),
      "If Wanted did not terminate the pending request with the expected ownership transfer");

  acquirer.rtiAmbassador().unpublishObjectClassAttributes(
      acquirerClass,
      acquirerAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  acquirer.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  acquirer.disconnect();
  owner.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
