#include "tck_ownership_denial_idle.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <mutex>
#include <stdexcept>
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

void scenarioOwnershipAcquisitionCancellationDenialRace(
    Options const& options,
    rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "ownership cancellation denial race requires evoked callbacks");
  require(!options.fom.empty(),
          "ownership cancellation denial race requires an adapter-supplied FOM");
  Session owner(options, model, "cancellation-denial-owner");
  Session requester(options, model, "cancellation-denial-requester");
  auto const federation = federationName(
      options, "ownership-acquisition-cancellation-denial-race");
  connectAndJoin(owner, requester, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.attributeName);
  require(ownerClass.isValid() && requesterClass.isValid() &&
              ownerAttribute.isValid() && requesterAttribute.isValid() &&
              ownerClass == requesterClass && ownerAttribute == requesterAttribute,
          "ownership cancellation denial race did not resolve matching handles");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass, requesterAttributes, true, L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(),
          "ownership cancellation denial race registration returned an invalid handle");
  waitFor(requester,
          [&] { return requester.recorder().hasDiscovery(object); },
          options, "ownership cancellation denial race discovery");
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass, requesterAttributes);
  for (int pass = 0; pass != 8; ++pass) {
    owner.pump();
    requester.pump();
  }
  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();

  std::vector<std::uint8_t> const acquisitionBytes{0xCAU, 0x25U};
  std::vector<std::uint8_t> const denialBytes{0xDEU, 0x25U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionBytes.data(), acquisitionBytes.size());
  rti::VariableLengthData const denialTag(
      denialBytes.data(), denialBytes.size());
  std::mutex coordinationMutex;
  std::condition_variable coordination;
  bool releaseCallbackEntered = false;
  bool cancellationCompleted = false;
  std::exception_ptr cancellationFailure;
  std::exception_ptr denialFailure;
  owner.recorder().setOwnershipReleaseHook(
      [&](rti::ObjectInstanceHandle const& callbackObject,
          rti::AttributeHandleSet const& callbackAttributes,
          rti::VariableLengthData const&) {
        {
          std::lock_guard<std::mutex> lock(coordinationMutex);
          releaseCallbackEntered = true;
        }
        coordination.notify_all();
        std::unique_lock<std::mutex> lock(coordinationMutex);
        bool const observedCancellation = coordination.wait_for(
            lock, std::chrono::milliseconds(options.timeoutMilliseconds),
            [&] { return cancellationCompleted; });
        lock.unlock();
        if (!observedCancellation) {
          denialFailure = std::make_exception_ptr(std::runtime_error(
              "ownership denial callback did not observe completed cancellation"));
          return;
        }
        try {
          owner.rtiAmbassador().attributeOwnershipReleaseDenied(
              callbackObject, callbackAttributes, denialTag);
        } catch (...) {
          denialFailure = std::current_exception();
        }
      });

  std::thread canceller([&] {
    std::unique_lock<std::mutex> lock(coordinationMutex);
    bool const observedRelease = coordination.wait_for(
        lock, std::chrono::milliseconds(options.timeoutMilliseconds),
        [&] { return releaseCallbackEntered; });
    lock.unlock();
    if (!observedRelease) {
      cancellationFailure = std::make_exception_ptr(std::runtime_error(
          "ownership cancellation did not observe the release callback"));
    } else {
      try {
        requester.rtiAmbassador().cancelAttributeOwnershipAcquisition(
            object, requesterAttributes);
      } catch (...) {
        cancellationFailure = std::current_exception();
      }
    }
    {
      std::lock_guard<std::mutex> completionLock(coordinationMutex);
      cancellationCompleted = true;
    }
    coordination.notify_all();
  });

  std::exception_ptr driverFailure;
  try {
    requester.rtiAmbassador().attributeOwnershipAcquisition(
        object, requesterAttributes, acquisitionTag);
    waitFor(owner,
            [&] {
              std::lock_guard<std::mutex> lock(coordinationMutex);
              return releaseCallbackEntered;
            },
            options, "ownership cancellation denial release callback");
  } catch (...) {
    driverFailure = std::current_exception();
  }
  canceller.join();
  owner.recorder().setOwnershipReleaseHook({});
  if (driverFailure) std::rethrow_exception(driverFailure);
  if (cancellationFailure) std::rethrow_exception(cancellationFailure);
  if (denialFailure) std::rethrow_exception(denialFailure);

  auto const release = owner.recorder().ownershipReleaseRequest();
  require(release.has_value() && release->object == object &&
              release->attributes == ownerAttributes &&
              release->tag == acquisitionBytes,
          "ownership denial race release request lost its object, set, or tag");
  waitFor(requester,
          [&] { return requester.recorder().ownershipUnavailable().has_value(); },
          options, "ownership cancellation denial unavailable reply");
  for (int pass = 0; pass != 8; ++pass) requester.pump();
  auto const unavailable = requester.recorder().ownershipUnavailable();
  require(unavailable->object == object &&
              unavailable->attributes == requesterAttributes &&
              unavailable->tag == denialBytes,
          "ownership denial race returned the wrong unavailable callback data");
  require(requester.recorder().ownershipUnavailableEvents().size() == 1U &&
              requester.recorder().ownershipAcquisitionCancellationEvents().empty() &&
              !requester.recorder().ownershipAcquisition().has_value(),
          "ownership denial race returned an extra or stale acquisition reply");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
              !requester.rtiAmbassador().isAttributeOwnedByFederate(
                  object, requesterAttribute),
          "ownership denial race changed the owner");

  requester.rtiAmbassador().unpublishObjectClassAttributes(
      requesterClass, requesterAttributes);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  requester.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioOwnershipAcquisitionCancellationDenialRaceContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipAcquisitionCancellationDenialRace(options, model);
}

void scenarioOwnershipAcquisitionCancellationMixedReplies(
    Options const& options,
    rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "mixed ownership cancellation replies require evoked callbacks");
  require(!options.fom.empty(),
          "mixed ownership cancellation replies require an adapter-supplied FOM");
  Session owner(options, model, "cancellation-mixed-replies-owner");
  Session requester(options, model, "cancellation-mixed-replies-requester");
  auto const federation = federationName(
      options, "ownership-acquisition-cancellation-mixed-replies");
  connectAndJoin(owner, requester, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerCancelledAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  auto const requesterCancelledAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.attributeName);
  auto const ownerTransferredAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.secondaryAttributeName);
  auto const requesterTransferredAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.secondaryAttributeName);
  auto const ownerDeniedAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.objectClassThirdAttributeName);
  auto const requesterDeniedAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass, options.objectClassThirdAttributeName);
  require(ownerClass.isValid() && requesterClass.isValid() &&
              ownerClass == requesterClass &&
              ownerCancelledAttribute.isValid() &&
              ownerTransferredAttribute.isValid() && ownerDeniedAttribute.isValid() &&
              requesterCancelledAttribute == ownerCancelledAttribute &&
              requesterTransferredAttribute == ownerTransferredAttribute &&
              requesterDeniedAttribute == ownerDeniedAttribute &&
              ownerCancelledAttribute != ownerTransferredAttribute &&
              ownerCancelledAttribute != ownerDeniedAttribute &&
              ownerTransferredAttribute != ownerDeniedAttribute,
          "mixed ownership cancellation fixture did not resolve three distinct matching attributes");

  rti::AttributeHandleSet const ownerAttributes{
      ownerCancelledAttribute, ownerTransferredAttribute, ownerDeniedAttribute};
  rti::AttributeHandleSet const requesterAttributes{
      requesterCancelledAttribute, requesterTransferredAttribute, requesterDeniedAttribute};
  rti::AttributeHandleSet const cancelledAttributes{requesterCancelledAttribute};
  rti::AttributeHandleSet const transferredAttributes{requesterTransferredAttribute};
  rti::AttributeHandleSet const deniedAttributes{requesterDeniedAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass, requesterAttributes, true, L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(),
          "mixed ownership cancellation registration returned an invalid handle");
  waitFor(requester,
          [&] { return requester.recorder().hasDiscovery(object); },
          options, "mixed ownership cancellation discovery");
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass, requesterAttributes);
  for (int pass = 0; pass != 8; ++pass) {
    owner.pump();
    requester.pump();
  }
  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();

  std::vector<std::uint8_t> const acquisitionBytes{0xCAU, 0x31U, 0x01U};
  std::vector<std::uint8_t> const transferBytes{0xD1U, 0x31U, 0x02U};
  std::vector<std::uint8_t> const denialBytes{0xDEU, 0x31U, 0x03U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionBytes.data(), acquisitionBytes.size());
  rti::VariableLengthData const transferTag(
      transferBytes.data(), transferBytes.size());
  rti::VariableLengthData const denialTag(
      denialBytes.data(), denialBytes.size());
  std::mutex coordinationMutex;
  std::condition_variable coordination;
  bool releaseCallbackEntered = false;
  bool cancellationCompleted = false;
  std::exception_ptr cancellationFailure;
  std::exception_ptr ownerResolutionFailure;
  owner.recorder().setOwnershipReleaseHook(
      [&](rti::ObjectInstanceHandle const& callbackObject,
          rti::AttributeHandleSet const& callbackAttributes,
          rti::VariableLengthData const&) {
        if (callbackObject != object || callbackAttributes != ownerAttributes) {
          ownerResolutionFailure = std::make_exception_ptr(std::runtime_error(
              "mixed ownership cancellation release callback returned the wrong set"));
          return;
        }
        {
          std::lock_guard<std::mutex> lock(coordinationMutex);
          releaseCallbackEntered = true;
        }
        coordination.notify_all();
        std::unique_lock<std::mutex> lock(coordinationMutex);
        bool const observedCancellation = coordination.wait_for(
            lock, std::chrono::milliseconds(options.timeoutMilliseconds),
            [&] { return cancellationCompleted; });
        lock.unlock();
        if (!observedCancellation) {
          ownerResolutionFailure = std::make_exception_ptr(std::runtime_error(
              "mixed ownership cancellation callback did not observe cancellation"));
          return;
        }
        try {
          rti::AttributeHandleSet divestedAttributes;
          owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
              callbackObject, transferredAttributes, transferTag, divestedAttributes);
          if (divestedAttributes != transferredAttributes) {
            ownerResolutionFailure = std::make_exception_ptr(std::runtime_error(
                "mixed ownership cancellation did not transfer the requested subset"));
            return;
          }
          owner.rtiAmbassador().attributeOwnershipReleaseDenied(
              callbackObject, deniedAttributes, denialTag);
        } catch (...) {
          ownerResolutionFailure = std::current_exception();
        }
      });

  std::thread canceller([&] {
    std::unique_lock<std::mutex> lock(coordinationMutex);
    bool const observedRelease = coordination.wait_for(
        lock, std::chrono::milliseconds(options.timeoutMilliseconds),
        [&] { return releaseCallbackEntered; });
    lock.unlock();
    if (!observedRelease) {
      cancellationFailure = std::make_exception_ptr(std::runtime_error(
          "mixed ownership cancellation did not observe the release callback"));
    } else {
      try {
        requester.rtiAmbassador().cancelAttributeOwnershipAcquisition(
            object, requesterAttributes);
      } catch (...) {
        cancellationFailure = std::current_exception();
      }
    }
    {
      std::lock_guard<std::mutex> completionLock(coordinationMutex);
      cancellationCompleted = true;
    }
    coordination.notify_all();
  });

  std::exception_ptr driverFailure;
  try {
    requester.rtiAmbassador().attributeOwnershipAcquisition(
        object, requesterAttributes, acquisitionTag);
    waitFor(owner,
            [&] { return owner.recorder().ownershipReleaseRequest().has_value(); },
            options, "mixed ownership cancellation release callback");
  } catch (...) {
    driverFailure = std::current_exception();
  }
  canceller.join();
  owner.recorder().setOwnershipReleaseHook({});
  if (driverFailure) std::rethrow_exception(driverFailure);
  if (cancellationFailure) std::rethrow_exception(cancellationFailure);
  if (ownerResolutionFailure) std::rethrow_exception(ownerResolutionFailure);

  auto const release = owner.recorder().ownershipReleaseRequest();
  require(release.has_value() && release->object == object &&
              release->attributes == ownerAttributes &&
              release->tag == acquisitionBytes,
          "mixed ownership cancellation request lost its object, attribute set, or tag");

  try {
    waitFor(requester,
            [&] {
              return requester.recorder().ownershipAcquisitionCancellation().has_value() &&
                  requester.recorder().ownershipAcquisition().has_value() &&
                  requester.recorder().ownershipUnavailable().has_value();
            },
            options, "mixed ownership cancellation reply forms");
  } catch (std::exception const& failure) {
    std::ostringstream details;
    details << failure.what()
            << "; observed reply counts: cancellation="
            << requester.recorder().ownershipAcquisitionCancellationEvents().size()
            << ", acquisition="
            << requester.recorder().ownershipAcquisitionEvents().size()
            << ", unavailable="
            << requester.recorder().ownershipUnavailableEvents().size();
    throw std::runtime_error(details.str());
  }
  for (int pass = 0; pass != 8; ++pass) requester.pump();

  auto const cancellation = requester.recorder().ownershipAcquisitionCancellation();
  auto const acquisition = requester.recorder().ownershipAcquisition();
  auto const unavailable = requester.recorder().ownershipUnavailable();
  require(cancellation->object == object &&
              cancellation->attributes == cancelledAttributes,
          "ownership cancellation confirmation did not contain only the canceled subset");
  require(acquisition->object == object &&
              acquisition->attributes == transferredAttributes &&
              acquisition->tag == transferBytes,
          "ownership acquisition notification did not contain only the transferred subset");
  require(unavailable->object == object &&
              unavailable->attributes == deniedAttributes &&
              unavailable->tag == denialBytes,
          "ownership unavailable reply did not contain only the denied subset");
  require(requester.recorder().ownershipAcquisitionCancellationEvents().size() == 1U &&
              requester.recorder().ownershipAcquisitionEvents().size() == 1U &&
              requester.recorder().ownershipUnavailableEvents().size() == 1U,
          "one cancellation invocation produced a duplicate or missing reply form");
  rti::AttributeHandleSet observedAttributes;
  observedAttributes.insert(cancellation->attributes.begin(), cancellation->attributes.end());
  observedAttributes.insert(acquisition->attributes.begin(), acquisition->attributes.end());
  observedAttributes.insert(unavailable->attributes.begin(), unavailable->attributes.end());
  require(observedAttributes == requesterAttributes,
          "the three cancellation reply forms did not cover exactly the requested attributes");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(
              object, ownerCancelledAttribute) &&
              requester.rtiAmbassador().isAttributeOwnedByFederate(
                  object, requesterTransferredAttribute) &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(
                  object, ownerDeniedAttribute) &&
              !requester.rtiAmbassador().isAttributeOwnedByFederate(
                  object, requesterCancelledAttribute) &&
              !requester.rtiAmbassador().isAttributeOwnedByFederate(
                  object, requesterDeniedAttribute),
          "mixed cancellation replies disagree with final ownership state");

  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  requester.resign(rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioOwnershipAcquisitionCancellationMixedRepliesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipAcquisitionCancellationMixedReplies(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
