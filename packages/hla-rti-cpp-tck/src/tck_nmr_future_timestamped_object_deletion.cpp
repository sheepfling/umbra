#include "tck_nmr_future_timestamped_object_deletion.hpp"

#include "tck_harness.hpp"
#include "tck_nmr_process_coordination.hpp"

#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

void requireNmrObjectDeletionPending(
    Session& receiver,
    std::string const& context) {
  auto const deadline = Clock::now() + std::chrono::milliseconds(40);
  while (Clock::now() < deadline) {
    receiver.pump();
    require(
        receiver.recorder().timedRemovals().empty() &&
            receiver.recorder().timeAdvanceGrants().empty(),
        context);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void runNmrFutureTimestampedObjectDeletionRequesterProcess(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& directory) {
  Session receiver(options, model, "owner");
  receiver.connect();
  auto const federation = federationName(
      options,
      "nmr-future-timestamped-object-deletion");
  receiver.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  writeNmrProcessMarker(directory, "federation-created.marker");
  receiver.join(options.ownerFederateName, options.federateType, federation);

  auto const objectClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = receiver.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "NMR process requester could not resolve its object class and attribute");
  rti::AttributeHandleSet const attributes{attribute};
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      objectClass,
      attributes,
      true,
      L"");

  auto receiverTime = makeTimeContext(receiver);
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMR process requester Time Constrained callback");
  writeNmrProcessMarker(directory, "requester-ready.marker");
  waitForNmrProcessMarker(directory, "publisher-ready.marker", options);
  waitForNmrProcessMarker(directory, "publisher-object-registered.marker", options);
  waitFor(
      receiver,
      [&] { return receiver.recorder().discoveryCount() == 1U; },
      options,
      "independent publisher's NMR object-deletion discovery");
  waitForNmrProcessMarker(directory, "publisher-ready-to-delete.marker", options);

  auto const requestBoundary = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      10U);
  auto const deletionTime = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      7U);
  receiver.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().nextMessageRequest(*requestBoundary);
  requireNmrObjectDeletionPending(
      receiver,
      "NMR process requester completed before the publisher sent its deletion");
  writeNmrProcessMarker(directory, "requester-pending.marker");

  waitForNmrProcessMarker(directory, "publisher-deletion-sent.marker", options);
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedRemovals().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "independent publisher's NMR timestamped object deletion");
  auto const removals = receiver.recorder().timedRemovals();
  auto const grants = receiver.recorder().timeAdvanceGrants();
  auto const discovery = receiver.recorder().discovery();
  std::vector<std::uint8_t> const tagBytes{0x4EU, 0x4DU, 0x52U};
  require(
      removals.size() == 1U && grants.size() == 1U && discovery.present &&
          removals.front().object == discovery.object &&
          removals.front().producer == discovery.producer,
      "NMR process requester received the wrong object-deletion identity");
  auto const& removal = removals.front();
  require(
      removal.tag == tagBytes,
      "NMR process requester did not preserve the deletion user tag");
  require(
      removal.time == encodeTime(*deletionTime) && !removal.timeText.empty() &&
          removal.sentOrder == rti::TIMESTAMP &&
          removal.receivedOrder == rti::TIMESTAMP,
      "NMR process requester received incorrect deletion time or order metadata");
  require(
      removal.retractionPresent && !removal.retraction.empty(),
      "NMR process requester omitted the deletion retraction metadata");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"remove", "grant"} &&
          grants.front().encoded == encodeTime(*deletionTime),
      "NMR process requester granted before removal or at the wrong time");

  auto queriedTime = receiverTime.factory->makeInitial();
  require(
      queriedTime != nullptr,
      "NMR process requester could not allocate Query Logical Time value");
  receiver.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(
      *queriedTime,
      *deletionTime,
      "NMR process requester final logical time");

  writeNmrProcessMarker(directory, "requester-deletion-received.marker");
  waitForNmrProcessMarker(directory, "publisher-resigned.marker", options);
  receiver.rtiAmbassador().disableTimeConstrained();
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      objectClass,
      attributes);
  receiver.resign(rti::NO_ACTION);
  receiver.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
}

void runNmrFutureTimestampedObjectDeletionPublisherProcess(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& directory) {
  waitForNmrProcessMarker(directory, "federation-created.marker", options);
  Session publisher(options, model, "member");
  publisher.connect();
  auto const federation = federationName(
      options,
      "nmr-future-timestamped-object-deletion");
  publisher.join(options.memberFederateName, options.federateType, federation);

  auto const objectClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = publisher.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "NMR process publisher could not resolve its object class and attribute");
  rti::AttributeHandleSet const attributes{attribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      objectClass,
      attributes);

  auto publisherTime = makeTimeContext(publisher);
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "NMR process publisher could not allocate its lookahead");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR process publisher Time Regulation callback");
  writeNmrProcessMarker(directory, "publisher-ready.marker");
  waitForNmrProcessMarker(directory, "requester-ready.marker", options);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(objectClass);
  require(object.isValid(), "NMR process publisher registered an invalid object");
  writeNmrProcessMarker(directory, "publisher-object-registered.marker");
  auto const publisherCurrentTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherCurrentTime);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "NMR process publisher initial advance");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherCurrentTime),
      "NMR process publisher advanced to an unexpected initial time");
  writeNmrProcessMarker(directory, "publisher-ready-to-delete.marker");
  waitForNmrProcessMarker(directory, "requester-pending.marker", options);

  auto const deletionTime = timeAfter(
      *publisherTime.factory,
      *publisherCurrentTime,
      *publisherTime.epsilon,
      5U);
  std::vector<std::uint8_t> const tagBytes{0x4EU, 0x4DU, 0x52U};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().deleteObjectInstance(
      object,
      tag,
      *deletionTime);
  require(
      retraction.isValid(),
      "NMR process publisher timestamped deletion had no retraction handle");
  writeNmrProcessMarker(directory, "publisher-deletion-sent.marker");

  auto const publisherAdvance = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAdvance);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "NMR process publisher final advance");
  require(
      publisher.recorder().timeAdvanceGrants().back().encoded ==
          encodeTime(*publisherAdvance),
      "NMR process publisher final advance reached the wrong time");
  waitForNmrProcessMarker(directory, "requester-deletion-received.marker", options);

  publisher.rtiAmbassador().disableTimeRegulation();
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      objectClass,
      attributes);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.disconnect();
  writeNmrProcessMarker(directory, "publisher-resigned.marker");
}

void runNmrFutureTimestampedObjectDeletion(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "NMR future timestamped object deletion requires an adapter-supplied logical-time implementation");

  if (!options.nmrProcessRole.empty()) {
    auto const directory = nextNmrProcessRunDirectory(options);
    if (options.nmrProcessRole == "requester") {
      runNmrFutureTimestampedObjectDeletionRequesterProcess(
          options,
          model,
          directory);
      return;
    }
    if (options.nmrProcessRole == "publisher") {
      runNmrFutureTimestampedObjectDeletionPublisherProcess(
          options,
          model,
          directory);
      return;
    }
    throw std::runtime_error(
        "NMR process role must be either requester or publisher");
  }
  require(
      options.nmrProcessCoordinationDirectory.empty(),
      "NMR process coordination directory requires an explicit process role");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "nmr-future-timestamped-object-deletion");

  publisher.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  receiver.join(options.memberFederateName, options.federateType, federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      publisherClass.isValid() && publisherAttribute.isValid() &&
          receiverClass.isValid() && receiverAttribute.isValid(),
      "NMR future timestamped object deletion lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "NMR future timestamped object deletion members selected different logical-time factories");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMR future timestamped object deletion time-constrained callback");

  auto const publisherLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto publisherLookahead = publisherTime.factory->makeZero();
  require(
      publisherLookahead != nullptr,
      "NMR future timestamped object deletion could not allocate publisher lookahead");
  publisherLookahead->setToDifference(
      *publisherLookaheadTime,
      *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR future timestamped object deletion time-regulation callback");

  auto const publisherCurrentTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherCurrentTime);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "NMR future timestamped object deletion regulator advance");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherCurrentTime),
      "NMR future timestamped object deletion regulator advanced to the wrong time");

  auto const deletionTime = timeAfter(
      *publisherTime.factory,
      *publisherCurrentTime,
      *publisherTime.epsilon,
      5U);
  auto const requestBoundary = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      10U);
  auto expectedGalt = receiverTime.factory->makeInitial();
  require(
      expectedGalt != nullptr,
      "NMR future timestamped object deletion could not allocate expected GALT");
  *expectedGalt = *deletionTime;
  auto actualGalt = receiverTime.factory->makeInitial();
  require(
      actualGalt != nullptr && receiver.rtiAmbassador().queryGALT(*actualGalt),
      "NMR future timestamped object deletion did not expose a defined GALT");
  requireTimeEquals(
      *actualGalt,
      *expectedGalt,
      "NMR future timestamped object deletion pre-request GALT");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "NMR future timestamped object deletion registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "NMR future timestamped object deletion object discovery");

  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().nextMessageRequest(*requestBoundary);
  receiver.pump();
  require(
      receiver.recorder().timedRemovals().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "NMR future timestamped object deletion completed before its future event arrived");

  std::vector<std::uint8_t> const tagBytes{0x4EU, 0x4DU, 0x52U};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().deleteObjectInstance(
      object,
      tag,
      *deletionTime);
  require(
      retraction.isValid(),
      "NMR future timestamped object deletion returned an invalid retraction handle");

  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedRemovals().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "NMR future timestamped object deletion callbacks");

  auto const removals = receiver.recorder().timedRemovals();
  auto const grants = receiver.recorder().timeAdvanceGrants();
  require(
      removals.size() == 1U && grants.size() == 1U,
      "NMR future timestamped object deletion returned duplicate or missing callbacks");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"remove", "grant"},
      "NMR future timestamped object deletion granted before its future removal");

  auto const& removal = removals.front();
  require(
      removal.object == object && removal.producer == publisher.federateHandle(),
      "NMR future timestamped object deletion returned the wrong removal identity");
  require(
      removal.tag == tagBytes,
      "NMR future timestamped object deletion did not preserve the user tag");
  require(
      removal.time == encodeTime(*deletionTime) && !removal.timeText.empty() &&
          removal.sentOrder == rti::TIMESTAMP &&
          removal.receivedOrder == rti::TIMESTAMP,
      "NMR future timestamped object deletion returned the wrong time or order metadata");
  require(
      removal.retractionPresent &&
          removal.retraction == copyBytes(retraction.encode()),
      "NMR future timestamped object deletion omitted or changed the retraction metadata");
  require(
      grants.front().encoded == encodeTime(*deletionTime),
      "NMR future timestamped object deletion returned the wrong grant time");
  auto queriedTime = receiverTime.factory->makeInitial();
  require(
      queriedTime != nullptr,
      "NMR future timestamped object deletion could not allocate queried logical time");
  receiver.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(
      *queriedTime,
      *deletionTime,
      "NMR future timestamped object deletion logical-time query");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioNmrFutureTimestampedObjectDeletion(
    Options const& options,
    rti::CallbackModel model) {
  runNmrFutureTimestampedObjectDeletion(options, model);
}

void scenarioNmrFutureTimestampedObjectDeletionContract(
    Options const& options,
    rti::CallbackModel model) {
  runNmrFutureTimestampedObjectDeletion(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
