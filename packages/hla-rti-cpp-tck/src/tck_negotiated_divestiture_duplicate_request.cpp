#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioNegotiatedDivestitureDuplicateRequest(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "duplicate negotiated divestiture coverage requires an adapter-supplied FOM");

  Session owner(options, model, "owner");
  Session acquirer(options, model, "member");
  auto const federation = federationName(
      options,
      "negotiated-divestiture-duplicate-request");
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
      "duplicate negotiated divestiture lookup returned an invalid handle");

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
  require(object.isValid(), "duplicate negotiated divestiture registration failed");
  waitFor(
      acquirer,
      [&] { return acquirer.recorder().hasDiscovery(object); },
      options,
      "duplicate negotiated divestiture object discovery");

  std::vector<std::uint8_t> const acquisitionTagBytes{0xA7U, 0x31U};
  std::vector<std::uint8_t> const firstOfferTagBytes{0xD7U, 0x31U};
  std::vector<std::uint8_t> const duplicateOfferTagBytes{0xD8U, 0x31U};
  std::vector<std::uint8_t> const confirmationTagBytes{0xE7U, 0x31U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  rti::VariableLengthData const firstOfferTag(
      firstOfferTagBytes.data(),
      firstOfferTagBytes.size());
  rti::VariableLengthData const duplicateOfferTag(
      duplicateOfferTagBytes.data(),
      duplicateOfferTagBytes.size());
  rti::VariableLengthData const confirmationTag(
      confirmationTagBytes.data(),
      confirmationTagBytes.size());

  acquirer.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      acquirerAttributes,
      acquisitionTag);
  owner.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
      object,
      ownerAttributes,
      firstOfferTag);
  requireException(
      [&] {
        owner.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
            object,
            ownerAttributes,
            duplicateOfferTag);
      },
      L"AttributeAlreadyBeingDivested",
      "a second negotiated divestiture for an attribute already being divested");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !acquirer.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              acquirerAttribute),
      "the rejected duplicate request changed current attribute ownership");

  std::vector<std::uint8_t> const ownerUpdateBytes{0x31U, 0xA7U};
  std::vector<std::uint8_t> const ownerUpdateTagBytes{0x32U, 0xA7U};
  rti::VariableLengthData const ownerUpdate(
      ownerUpdateBytes.data(),
      ownerUpdateBytes.size());
  rti::VariableLengthData const ownerUpdateTag(
      ownerUpdateTagBytes.data(),
      ownerUpdateTagBytes.size());
  rti::AttributeHandleValueMap const ownerValues{{ownerAttribute, ownerUpdate}};
  owner.rtiAmbassador().updateAttributeValues(
      object,
      ownerValues,
      ownerUpdateTag);
  waitFor(
      acquirer,
      [&] { return acquirer.recorder().reflection().present; },
      options,
      "the current owner's update while negotiated divestiture is pending");
  auto const reflection = acquirer.recorder().reflection();
  auto const reflectedValue = reflection.values.find(acquirerAttribute);
  require(
      reflection.object == object &&
          reflectedValue != reflection.values.end() &&
          copyBytes(reflectedValue->second) == ownerUpdateBytes &&
          reflection.tag == ownerUpdateTagBytes &&
          reflection.producer == owner.federateHandle() &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
      "the current owner's update responsibility ended before negotiated divestiture completed");

  waitFor(
      owner,
      [&] { return owner.recorder().divestitureConfirmation().has_value(); },
      options,
      "the original negotiated-divestiture confirmation request");
  auto const confirmationRequest = owner.recorder().divestitureConfirmation();
  require(
      confirmationRequest->object == object &&
          confirmationRequest->attributes == ownerAttributes &&
          confirmationRequest->tag == acquisitionTagBytes,
      "the rejected duplicate replaced the original confirmation request");

  owner.rtiAmbassador().confirmDivestiture(
      object,
      ownerAttributes,
      confirmationTag);
  waitFor(
      acquirer,
      [&] { return acquirer.recorder().ownershipAcquisition().has_value(); },
      options,
      "the original negotiated-divestiture acquisition notification");
  auto const acquisition = acquirer.recorder().ownershipAcquisition();
  require(
      acquisition->object == object &&
          acquisition->attributes == acquirerAttributes &&
          acquisition->tag == confirmationTagBytes &&
          !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          acquirer.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              acquirerAttribute),
      "the original negotiated divestiture did not complete after duplicate rejection");

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
