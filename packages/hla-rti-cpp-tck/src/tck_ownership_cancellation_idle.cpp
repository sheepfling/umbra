#include "tck_ownership_cancellation_idle.hpp"

#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioOwnershipCancellationIdle(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "idle ownership-cancellation testing requires an adapter-supplied FOM");

  Session owner(options, model, "idle-cancellation-owner");
  Session requester(options, model, "idle-cancellation-requester");
  auto const federation = federationName(options, "ownership-cancellation-idle");
  connectAndJoin(owner, requester, options, federation, options.modelFom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.typedIdentityAttributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.typedIdentityAttributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() &&
          ownerAttribute.isValid() && requesterAttribute.isValid() &&
          ownerClass == requesterClass && ownerAttribute == requesterAttribute,
      "idle ownership-cancellation fixture did not resolve the same standard handles");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};

  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass, requesterAttributes, true, L"");
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "idle ownership-cancellation registration returned an invalid handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "idle ownership-cancellation discovery");
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass, requesterAttributes);

  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> const tagBytes{0xC2U, 0x11U, 0x6DU, 0x09U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object, requesterAttributes, tag);
  waitFor(
      owner,
      [&] { return owner.recorder().ownershipReleaseRequest().has_value(); },
      options,
      "idle ownership-cancellation owner release request");
  auto const release = owner.recorder().ownershipReleaseRequest();
  require(
      release->object == object && release->attributes == ownerAttributes &&
          release->tag == tagBytes,
      "idle ownership-cancellation release request lost its object, attribute, or tag");

  requester.rtiAmbassador().cancelAttributeOwnershipAcquisition(
      object, requesterAttributes);
  if (model == rti::HLA_IMMEDIATE) {
    // Do not call any requester RTI service while observing the pushed callback.
    auto const deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(options.timeoutMilliseconds);
    while (!requester.recorder().ownershipAcquisitionCancellation().has_value() &&
           std::chrono::steady_clock::now() < deadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  } else {
    waitFor(
        requester,
        [&] {
          return requester.recorder().ownershipAcquisitionCancellation().has_value();
        },
        options,
        "evoked ownership-cancellation confirmation");
  }
  auto const confirmation = requester.recorder().ownershipAcquisitionCancellation();
  require(
      confirmation.has_value() && confirmation->object == object &&
          confirmation->attributes == requesterAttributes &&
          confirmation->tag.empty(),
      "idle ownership cancellation did not push one matching confirmation");
  require(
      requester.recorder().ownershipAcquisitionCancellationEvents().size() == 1U &&
          !requester.recorder().ownershipAcquisition().has_value(),
      "idle ownership cancellation duplicated confirmation or acquired ownership");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              object, requesterAttribute),
      "idle ownership cancellation changed the owner");

  requester.resign(rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioOwnershipCancellationIdleContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipCancellationIdle(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
