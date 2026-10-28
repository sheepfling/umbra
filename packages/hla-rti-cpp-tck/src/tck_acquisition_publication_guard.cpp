#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioAcquisitionPublicationGuardUnavailable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Acquisition publication-guard testing requires an adapter-supplied FOM");

  Session owner(options, model, "publication-guard-owner");
  Session requester(options, model, "publication-guard-requester");
  auto const federation = federationName(
      options,
      "acquisition-publication-guard-unavailable");
  connectAndJoin(owner, requester, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.attributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() &&
          ownerAttribute.isValid() && requesterAttribute.isValid(),
      "Acquisition publication-guard lookup returned an invalid standard handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Acquisition publication-guard registration returned an invalid handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "acquisition publication-guard object discovery");

  std::vector<std::uint8_t> const acquisitionTagBytes{0xA5U, 0x01U};
  std::vector<std::uint8_t> const denialTagBytes{0xD3U, 0x02U};
  std::vector<std::uint8_t> const divestitureTagBytes{0xE7U, 0x03U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  rti::VariableLengthData const denialTag(
      denialTagBytes.data(),
      denialTagBytes.size());
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(),
      divestitureTagBytes.size());
  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();

  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      requesterAttributes,
      acquisitionTag);
  waitFor(
      owner,
      [&] { return owner.recorder().ownershipReleaseRequest().has_value(); },
      options,
      "acquisition publication-guard owner release request");
  auto const releaseRequest = owner.recorder().ownershipReleaseRequest();
  require(
      releaseRequest->object == object &&
          releaseRequest->attributes == ownerAttributes &&
          releaseRequest->tag == acquisitionTagBytes,
      "Acquisition publication-guard release request returned wrong metadata");
  requireException(
      [&] {
        requester.rtiAmbassador().unpublishObjectClassAttributes(
            requesterClass,
            requesterAttributes);
      },
      L"OwnershipAcquisitionPending",
      "stopping publication before the ownership-unavailable callback");

  owner.rtiAmbassador().attributeOwnershipReleaseDenied(
      object,
      ownerAttributes,
      denialTag);
  if (model == rti::HLA_EVOKED) {
    requireException(
        [&] {
          requester.rtiAmbassador().unpublishObjectClassAttributes(
              requesterClass,
              requesterAttributes);
        },
        L"OwnershipAcquisitionPending",
        "stopping publication before evoking ownership unavailable");
  }
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipUnavailable().has_value(); },
      options,
      "acquisition publication-guard unavailable callback");
  auto const unavailable = requester.recorder().ownershipUnavailable();
  require(
      unavailable->object == object &&
          unavailable->attributes == requesterAttributes &&
          unavailable->tag == denialTagBytes,
      "Acquisition publication-guard unavailable callback returned wrong metadata");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              requesterAttribute),
      "Ownership-unavailable callback changed ownership");

  requester.rtiAmbassador().unpublishObjectClassAttributes(
      requesterClass,
      requesterAttributes);

  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  requester.recorder().clearOwnershipRecords();
  owner.recorder().clearOwnershipRecords();
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      requesterAttributes,
      acquisitionTag);
  waitFor(
      owner,
      [&] { return owner.recorder().ownershipReleaseRequest().has_value(); },
      options,
      "acquisition publication-guard notification release request");
  requireException(
      [&] {
        requester.rtiAmbassador().unpublishObjectClassAttributes(
            requesterClass,
            requesterAttributes);
      },
      L"OwnershipAcquisitionPending",
      "stopping publication before acquisition notification");

  rti::AttributeHandleSet divestedAttributes;
  owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object,
      ownerAttributes,
      divestitureTag,
      divestedAttributes);
  require(
      divestedAttributes == ownerAttributes &&
          !owner.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              ownerAttribute) &&
          requester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              requesterAttribute),
      "If-Wanted divestiture did not transfer the requested attribute");
  if (model == rti::HLA_EVOKED) {
    requireException(
        [&] {
          requester.rtiAmbassador().unpublishObjectClassAttributes(
              requesterClass,
              requesterAttributes);
        },
        L"OwnershipAcquisitionPending",
        "stopping publication before evoking acquisition notification");
  }
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipAcquisition().has_value(); },
      options,
      "acquisition publication-guard acquisition notification");
  auto const acquisition = requester.recorder().ownershipAcquisition();
  require(
      acquisition->object == object &&
          acquisition->attributes == requesterAttributes &&
          acquisition->tag == divestitureTagBytes,
      "Acquisition notification returned incorrect object, attributes, or tag");
  requester.rtiAmbassador().unpublishObjectClassAttributes(
      requesterClass,
      requesterAttributes);

  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  requester.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioAcquisitionPublicationGuardUnavailableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAcquisitionPublicationGuardUnavailable(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
