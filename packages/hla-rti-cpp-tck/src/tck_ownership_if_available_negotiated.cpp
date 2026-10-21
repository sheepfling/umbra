#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioIfAvailableDuringNegotiatedDivestiture(
    Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Negotiated If Available testing requires an adapter-supplied FOM");
  require(model == rti::HLA_EVOKED,
          "Negotiated If Available eligibility requires evoked callback ordering");
  Session owner(options, model, "negotiated-if-available-owner");
  Session requester(options, model, "negotiated-if-available-requester");
  auto const federation = federationName(options, "negotiated-if-available");
  owner.connect();
  requester.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName + L"-negotiated-if-available-owner",
             options.federateType, federation);
  requester.join(options.memberFederateName + L"-negotiated-if-available-requester",
                 options.federateType, federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.attributeName);
  require(ownerClass.isValid() && requesterClass.isValid() &&
              ownerAttribute.isValid() && requesterAttribute.isValid(),
          "Adapter FOM did not resolve the negotiated If Available class and attribute");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass, requesterAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass, requesterAttributes, true, L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "Negotiated If Available registration returned an invalid handle");
  waitFor(requester,
          [&] { return requester.recorder().hasDiscovery(object); },
          options, "negotiated If Available object discovery");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "Registrant did not initially own the negotiated attribute");

  std::vector<std::uint8_t> const requestTagBytes{0x6AU, 0x71U};
  std::vector<std::uint8_t> const divestitureTagBytes{0x6BU, 0x71U};
  std::vector<std::uint8_t> const confirmationTagBytes{0x6CU, 0x71U};
  rti::VariableLengthData const requestTag(
      requestTagBytes.data(), requestTagBytes.size());
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(), divestitureTagBytes.size());
  rti::VariableLengthData const confirmationTag(
      confirmationTagBytes.data(), confirmationTagBytes.size());
  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();

  // Keep the requester's If Available callback queued until the owner begins
  // negotiation, matching the native evoked-ordering witness.
  requester.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object, requesterAttributes, requestTag);
  owner.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
      object, ownerAttributes, divestitureTag);
  waitFor(owner,
          [&] { return owner.recorder().divestitureConfirmation().has_value(); },
          options, "negotiated If Available divestiture confirmation request");
  auto const offer = owner.recorder().divestitureConfirmation();
  require(offer->object == object && offer->attributes == ownerAttributes &&
              offer->tag == requestTagBytes,
          "Negotiated If Available confirmation request lost the requester set or tag");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
              !requester.rtiAmbassador().isAttributeOwnedByFederate(
                  object, requesterAttribute) &&
              !requester.recorder().ownershipAcquisition().has_value(),
          "Negotiated If Available transferred ownership before owner confirmation");

  owner.rtiAmbassador().confirmDivestiture(
      object, ownerAttributes, confirmationTag);
  require(!owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
              requester.rtiAmbassador().isAttributeOwnedByFederate(
                  object, requesterAttribute),
          "Negotiated If Available did not transfer ownership at confirmation");
  waitFor(requester,
          [&] { return requester.recorder().ownershipAcquisition().has_value(); },
          options, "negotiated If Available acquisition notification");
  auto const acquisition = requester.recorder().ownershipAcquisition();
  require(acquisition->object == object &&
              acquisition->attributes == requesterAttributes &&
              acquisition->tag == confirmationTagBytes,
          "Negotiated If Available notification returned the wrong object, set, or tag");

  owner.rtiAmbassador().deleteObjectInstance(object, rti::VariableLengthData{});
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioIfAvailableDuringNegotiatedDivestitureContract(
    Options const& options, rti::CallbackModel model) {
  scenarioIfAvailableDuringNegotiatedDivestiture(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
