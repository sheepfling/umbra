#include <cstdint>
#include <vector>

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioNegotiatedDivestitureConfirmationFlow(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Negotiated-divestiture testing requires an adapter-supplied FOM");

  Session owner(options, model, "negotiated-divestiture-owner");
  Session acquirer(options, model, "negotiated-divestiture-acquirer");
  auto const federation = federationName(
      options,
      "negotiated-divestiture-confirmation-flow");
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
      "Negotiated-divestiture lookup returned an invalid standard handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const acquirerAttributes{acquirerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  owner.rtiAmbassador().subscribeObjectClassAttributes(
      ownerClass,
      ownerAttributes,
      true,
      L"");
  acquirer.rtiAmbassador().publishObjectClassAttributes(
      acquirerClass,
      acquirerAttributes);
  acquirer.rtiAmbassador().subscribeObjectClassAttributes(
      acquirerClass,
      acquirerAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Negotiated-divestiture registration returned an invalid object handle");
  waitFor(
      acquirer,
      [&] { return acquirer.recorder().hasDiscovery(object); },
      options,
      "negotiated-divestiture object discovery");

  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !acquirer.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              acquirerAttribute),
      "Negotiated-divestiture setup did not establish the standard initial ownership state");

  std::vector<std::uint8_t> const divestitureTagBytes{0x10U, 0xD1U};
  std::vector<std::uint8_t> const acquisitionRequestTagBytes{0x20U, 0xA2U};
  std::vector<std::uint8_t> const confirmationTagBytes{0x30U, 0xC3U};
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(),
      divestitureTagBytes.size());
  rti::VariableLengthData const acquisitionRequestTag(
      acquisitionRequestTagBytes.data(),
      acquisitionRequestTagBytes.size());
  rti::VariableLengthData const confirmationTag(
      confirmationTagBytes.data(),
      confirmationTagBytes.size());

  owner.recorder().clearOwnershipRecords();
  acquirer.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
      object,
      ownerAttributes,
      divestitureTag);
  acquirer.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      acquirerAttributes,
      acquisitionRequestTag);

  waitFor(
      owner,
      [&] { return owner.recorder().divestitureConfirmation().has_value(); },
      options,
      "negotiated-divestiture confirmation request");
  auto const confirmationRequest = owner.recorder().divestitureConfirmation();
  require(
      confirmationRequest->object == object &&
          confirmationRequest->attributes == ownerAttributes &&
          confirmationRequest->tag == acquisitionRequestTagBytes,
      "Negotiated-divestiture confirmation request returned the wrong object, attributes, or acquisition tag");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !acquirer.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              acquirerAttribute) &&
          !acquirer.recorder().ownershipAcquisition().has_value(),
      "Negotiated divestiture transferred ownership before confirmation");

  owner.rtiAmbassador().confirmDivestiture(
      object,
      ownerAttributes,
      confirmationTag);
  waitFor(
      acquirer,
      [&] { return acquirer.recorder().ownershipAcquisition().has_value(); },
      options,
      "negotiated-divestiture acquisition notification");
  auto const acquisition = acquirer.recorder().ownershipAcquisition();
  require(
      acquisition->object == object &&
          acquisition->attributes == acquirerAttributes &&
          acquisition->tag == confirmationTagBytes,
      "Negotiated-divestiture acquisition notification returned the wrong object, attributes, or confirmation tag");
  require(
      !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          acquirer.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              acquirerAttribute),
      "Confirm Divestiture did not transfer ownership to the acquirer");

  std::vector<std::uint8_t> const transferredValue{0x51U, 0x88U};
  std::vector<std::uint8_t> const transferredUpdateTagBytes{0x52U, 0x89U};
  rti::VariableLengthData const transferredUpdateTag(
      transferredUpdateTagBytes.data(),
      transferredUpdateTagBytes.size());
  rti::AttributeHandleValueMap transferredValues;
  transferredValues.emplace(
      acquirerAttribute,
      rti::VariableLengthData(
          transferredValue.data(),
          transferredValue.size()));
  acquirer.rtiAmbassador().updateAttributeValues(
      object,
      transferredValues,
      transferredUpdateTag);

  waitFor(
      owner,
      [&] { return owner.recorder().reflection().present; },
      options,
      "post-transfer attribute reflection to the former owner");
  auto const reflected = owner.recorder().reflection();
  require(
      reflected.object == object && reflected.values.size() == 1U &&
          reflected.values.count(ownerAttribute) == 1U &&
          copyBytes(reflected.values.at(ownerAttribute)) == transferredValue &&
          reflected.tag == transferredUpdateTagBytes &&
          reflected.producer == acquirer.federateHandle(),
      "Post-transfer update did not reflect the new owner's value, tag, and identity");

  acquirer.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object,
      acquirerAttributes,
      rti::VariableLengthData{});
  acquirer.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  acquirer.disconnect();
  owner.disconnect();
}

void scenarioNegotiatedDivestitureConfirmationFlowContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNegotiatedDivestitureConfirmationFlow(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
