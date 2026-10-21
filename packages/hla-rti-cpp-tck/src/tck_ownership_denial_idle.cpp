#include "tck_ownership_denial_idle.hpp"

#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {

template <typename Predicate>
void awaitOwnershipCallback(
    Session& session,
    Predicate predicate,
    Options const& options,
    rti::CallbackModel model,
    std::string const& description) {
  if (model == rti::HLA_EVOKED) {
    waitFor(session, predicate, options, description);
    return;
  }
  // The immediate recipient remains idle: observation never calls its RTI.
  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (!predicate() && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(predicate(), "timed out waiting for " + description);
}

void scenarioOwnershipDenialIdle(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.fom.empty(), "idle ownership-denial testing requires an adapter-supplied FOM");
  Session owner(options, model, "idle-denial-owner");
  Session requester(options, model, "idle-denial-requester");
  auto const federation = federationName(options, "ownership-denial-idle");
  connectAndJoin(owner, requester, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.attributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() &&
          ownerAttribute.isValid() && requesterAttribute.isValid() &&
          ownerClass == requesterClass && ownerAttribute == requesterAttribute,
      "idle ownership-denial fixture did not resolve matching standard handles");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass, requesterAttributes, true, L"");
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "idle ownership-denial registration returned an invalid handle");
  awaitOwnershipCallback(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options, model, "idle ownership-denial discovery");
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass, requesterAttributes);

  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> const acquisitionBytes{0xC2U, 0x11U, 0x6DU, 0x09U};
  std::vector<std::uint8_t> const denialBytes{0xDAU, 0x7AU, 0x01U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionBytes.data(), acquisitionBytes.size());
  rti::VariableLengthData const denialTag(denialBytes.data(), denialBytes.size());
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object, requesterAttributes, acquisitionTag);
  awaitOwnershipCallback(
      owner,
      [&] { return owner.recorder().ownershipReleaseRequest().has_value(); },
      options, model, "idle ownership-denial owner release request");
  auto const release = owner.recorder().ownershipReleaseRequest();
  require(
      release->object == object && release->attributes == ownerAttributes &&
          release->tag == acquisitionBytes,
      "idle ownership-denial release request lost its object, attribute, or tag");
  owner.rtiAmbassador().attributeOwnershipReleaseDenied(
      object, ownerAttributes, denialTag);
  awaitOwnershipCallback(
      requester,
      [&] { return requester.recorder().ownershipUnavailable().has_value(); },
      options, model, "idle ownership-denial unavailable callback");
  auto const unavailable = requester.recorder().ownershipUnavailable();
  require(
      unavailable->object == object &&
          unavailable->attributes == requesterAttributes &&
          unavailable->tag == denialBytes,
      "idle ownership denial did not return the requested object, attribute, and denial tag");
  require(
      requester.recorder().ownershipUnavailableEvents().size() == 1U &&
          !requester.recorder().ownershipAcquisition().has_value(),
      "idle ownership denial duplicated unavailability or acquired ownership");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              object, requesterAttribute),
      "idle ownership denial changed the owner");

  requester.resign(rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioOwnershipDenialIdleContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipDenialIdle(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
