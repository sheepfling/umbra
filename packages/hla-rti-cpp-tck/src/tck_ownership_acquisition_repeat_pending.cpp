#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioRepeatAcquisitionWhilePending(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.multiAttributeFom.empty(),
          "Repeated acquisition requires an adapter-supplied two-attribute FOM");

  Session owner(options, model, "repeat-pending-owner");
  Session requester(options, model, "repeat-pending-requester");
  auto const federation = federationName(options, "repeat-acquisition-pending");
  connectAndJoin(owner, requester, options, federation, options.multiAttributeFom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const ownerFirst = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.multiAttributeFirstName);
  auto const ownerSecond = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.multiAttributeSecondName);
  auto const requesterFirst = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.multiAttributeFirstName);
  auto const requesterSecond = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.multiAttributeSecondName);
  require(ownerClass.isValid() && requesterClass.isValid() &&
              ownerFirst.isValid() && ownerSecond.isValid() &&
              requesterFirst.isValid() && requesterSecond.isValid() &&
              ownerFirst != ownerSecond && requesterFirst != requesterSecond,
          "Adapter FOM did not resolve two distinct acquisition attributes");

  rti::AttributeHandleSet const ownerFirstOnly{ownerFirst};
  rti::AttributeHandleSet const ownerSecondOnly{ownerSecond};
  rti::AttributeHandleSet const ownerBoth{ownerFirst, ownerSecond};
  rti::AttributeHandleSet const requesterFirstOnly{requesterFirst};
  rti::AttributeHandleSet const requesterSecondOnly{requesterSecond};
  rti::AttributeHandleSet const requesterBoth{requesterFirst, requesterSecond};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerBoth);
  requester.rtiAmbassador().publishObjectClassAttributes(requesterClass, requesterBoth);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass, requesterBoth, true, L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "Repeated acquisition registered an invalid object");
  waitFor(requester,
          [&] { return requester.recorder().hasDiscovery(object); },
          options,
          "repeated acquisition object discovery");

  struct ReleaseRequestObservation final {
    rti::ObjectInstanceHandle object;
    rti::AttributeHandleSet attributes;
    std::vector<std::uint8_t> tag;
  };
  std::vector<ReleaseRequestObservation> releaseRequests;
  owner.recorder().setOwnershipReleaseHook(
      [&](rti::ObjectInstanceHandle const& requestedObject,
          rti::AttributeHandleSet const& attributes,
          rti::VariableLengthData const& tag) {
        releaseRequests.push_back(
            ReleaseRequestObservation{requestedObject, attributes, copyBytes(tag)});
      });

  std::vector<std::uint8_t> firstTagBytes{0x31U, 0x41U};
  std::vector<std::uint8_t> repeatedTagBytes{0x59U, 0x26U};
  rti::VariableLengthData const firstTag(
      firstTagBytes.data(), firstTagBytes.size());
  rti::VariableLengthData const repeatedTag(
      repeatedTagBytes.data(), repeatedTagBytes.size());

  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object, requesterFirstOnly, firstTag);
  waitFor(owner,
          [&] { return releaseRequests.size() >= 1U; },
          options,
          "first pending acquisition release request");
  require(releaseRequests.size() == 1U &&
              releaseRequests.front().object == object &&
              releaseRequests.front().attributes == ownerFirstOnly &&
              releaseRequests.front().tag == firstTagBytes,
          "First acquisition did not request release of only its attribute");

  // The first attribute is already Acquiring. Repeating the request with a
  // second attribute must preserve that state and only request release for
  // the newly included attribute.
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object, requesterBoth, repeatedTag);
  waitFor(owner,
          [&] { return releaseRequests.size() >= 2U; },
          options,
          "newly included acquisition release request");
  for (int pass = 0; pass != 8; ++pass) {
    owner.pump();
  }
  require(releaseRequests.size() == 2U,
          "Repeating acquisition issued a duplicate release request for the pending attribute");
  require(releaseRequests[1].object == object &&
              releaseRequests[1].attributes == ownerSecondOnly &&
              releaseRequests[1].tag == repeatedTagBytes,
          "Repeated acquisition did not start only the newly included attribute");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerSecond) &&
              !requester.rtiAmbassador().isAttributeOwnedByFederate(object, requesterFirst) &&
              !requester.rtiAmbassador().isAttributeOwnedByFederate(object, requesterSecond),
          "Repeated acquisition changed ownership before either request was resolved");

  requester.rtiAmbassador().cancelAttributeOwnershipAcquisition(object, requesterBoth);
  waitFor(requester,
          [&] {
            return requester.recorder().ownershipAcquisitionCancellation().has_value();
          },
          options,
          "confirmation of cancellation for both pending attributes");
  auto const cancellation = requester.recorder().ownershipAcquisitionCancellation();
  require(cancellation->object == object && cancellation->attributes == requesterBoth,
          "Cancellation did not recognize both still-pending acquisition attributes");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerSecond),
          "Canceling repeated acquisition changed the owner's attribute state");

  // Exercise the second state named in §7.8. A's cancellation confirmation
  // is deliberately left undelivered while the repeated request is issued.
  releaseRequests.clear();
  requester.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> cancelingTagBytes{0x70U, 0x71U};
  std::vector<std::uint8_t> repeatedWhileCancelingTagBytes{0x72U, 0x73U};
  rti::VariableLengthData const cancelingTag(
      cancelingTagBytes.data(), cancelingTagBytes.size());
  rti::VariableLengthData const repeatedWhileCancelingTag(
      repeatedWhileCancelingTagBytes.data(),
      repeatedWhileCancelingTagBytes.size());

  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object, requesterFirstOnly, cancelingTag);
  waitFor(owner,
          [&] { return releaseRequests.size() >= 1U; },
          options,
          "acquisition release request before cancellation");
  require(releaseRequests.size() == 1U &&
              releaseRequests.front().attributes == ownerFirstOnly &&
              releaseRequests.front().tag == cancelingTagBytes,
          "Pre-cancellation request did not target only attribute A");

  if (model == rti::HLA_IMMEDIATE) {
    requester.rtiAmbassador().disableCallbacks();
  }
  requester.rtiAmbassador().cancelAttributeOwnershipAcquisition(
      object, requesterFirstOnly);
  // No requester callback is pumped before this invocation. In HLA_IMMEDIATE
  // the callback is explicitly held; in HLA_EVOKED it remains in the queue.
  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object, requesterBoth, repeatedWhileCancelingTag);
  waitFor(owner,
          [&] { return releaseRequests.size() >= 2U; },
          options,
          "new attribute release request while A is being canceled");
  for (int pass = 0; pass != 8; ++pass) {
    owner.pump();
  }
  require(releaseRequests.size() == 2U &&
              releaseRequests[1].attributes == ownerSecondOnly &&
              releaseRequests[1].tag == repeatedWhileCancelingTagBytes,
          "Trying-to-cancel acquisition was replaced or generated a duplicate release request");

  if (model == rti::HLA_IMMEDIATE) {
    requester.rtiAmbassador().enableCallbacks();
  }
  waitFor(requester,
          [&] {
            return requester.recorder()
                       .ownershipAcquisitionCancellationEvents()
                       .size() >= 1U;
          },
          options,
          "cancellation confirmation for A after repeated acquisition");
  auto cancellations =
      requester.recorder().ownershipAcquisitionCancellationEvents();
  require(cancellations.size() == 1U &&
              cancellations.front().object == object &&
              cancellations.front().attributes == requesterFirstOnly,
          "Repeated acquisition changed the cancellation state for A");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerSecond) &&
              !requester.rtiAmbassador().isAttributeOwnedByFederate(object, requesterFirst) &&
              !requester.rtiAmbassador().isAttributeOwnedByFederate(object, requesterSecond),
          "Resolving A's cancellation changed ownership of either attribute");

  requester.rtiAmbassador().cancelAttributeOwnershipAcquisition(
      object, requesterSecondOnly);
  waitFor(requester,
          [&] {
            return requester.recorder()
                       .ownershipAcquisitionCancellationEvents()
                       .size() >= 2U;
          },
          options,
          "cancellation confirmation for newly acquired B");
  cancellations = requester.recorder().ownershipAcquisitionCancellationEvents();
  require(cancellations.size() == 2U &&
              cancellations[1].object == object &&
              cancellations[1].attributes == requesterSecondOnly,
          "Newly included attribute B did not remain independently cancelable");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerSecond),
          "Canceling the new acquisition changed the owner's ownership");

  requester.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioRepeatAcquisitionWhilePendingContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRepeatAcquisitionWhilePending(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
