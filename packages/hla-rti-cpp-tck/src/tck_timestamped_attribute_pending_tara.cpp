#include "tck_timestamped_attribute_pending_tara.hpp"

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

void requireTaraAttributeUpdatePending(
    Session& receiver,
    std::string const& context) {
  auto const deadline = Clock::now() + std::chrono::milliseconds(40);
  while (Clock::now() < deadline) {
    receiver.pump();
    require(
        receiver.recorder().timedReflections().empty() &&
            receiver.recorder().timeAdvanceGrants().empty(),
        context);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void runTaraFutureTimestampedAttributeUpdateRequesterProcess(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& directory) {
  Session receiver(options, model, "owner");
  receiver.connect();
  auto const federation = federationName(
      options,
      "tara-future-timestamped-attribute-update");
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
      "TARA process requester could not resolve its object class and attribute");
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
      "TARA process requester Time Constrained callback");
  writeNmrProcessMarker(directory, "requester-ready.marker");
  waitForNmrProcessMarker(directory, "publisher-ready.marker", options);
  waitForNmrProcessMarker(directory, "publisher-object-registered.marker", options);
  waitFor(
      receiver,
      [&] { return receiver.recorder().discoveryCount() == 1U; },
      options,
      "independent publisher's TARA attribute-update object discovery");
  waitForNmrProcessMarker(directory, "publisher-ready-to-update.marker", options);

  auto const updateTime = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      5U);
  auto const requestBoundary = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      6U);
  receiver.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().timeAdvanceRequestAvailable(*requestBoundary);
  requireTaraAttributeUpdatePending(
      receiver,
      "TARA process requester completed before the publisher sent its update");
  writeNmrProcessMarker(directory, "requester-pending.marker");

  waitForNmrProcessMarker(directory, "publisher-update-sent.marker", options);
  requireTaraAttributeUpdatePending(
      receiver,
      "TARA process requester delivered the future update before the publisher advanced");
  writeNmrProcessMarker(directory, "requester-update-queued.marker");
  waitForNmrProcessMarker(directory, "publisher-advanced.marker", options);

  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedReflections().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "independent publisher's TARA timestamped attribute update");
  auto const reflections = receiver.recorder().timedReflections();
  auto const grants = receiver.recorder().timeAdvanceGrants();
  auto const discovery = receiver.recorder().discovery();
  std::vector<std::uint8_t> const valueBytes{'T', 'A', 'R', 'A', '5'};
  std::vector<std::uint8_t> const tagBytes{'T', 'A', 'R', 'A'};
  require(
      reflections.size() == 1U && grants.size() == 1U && discovery.present &&
          reflections.front().object == discovery.object &&
          reflections.front().producer == discovery.producer,
      "TARA process requester received the wrong reflection identity");
  auto const& reflection = reflections.front();
  require(
      reflection.values.size() == 1U &&
          reflection.values.count(attribute) == 1U &&
          copyBytes(reflection.values.at(attribute)) == valueBytes &&
          reflection.tag == tagBytes,
      "TARA process requester received the wrong attribute value or tag");
  require(
      reflection.time == encodeTime(*updateTime) &&
          !reflection.timeText.empty() &&
          reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP &&
          reflection.retractionPresent && !reflection.retraction.empty() &&
          reflection.transportation.isValid(),
      "TARA process requester received incorrect reflection metadata");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"} &&
          grants.front().encoded == encodeTime(*requestBoundary),
      "TARA process requester granted before reflection or at the wrong time");

  auto queriedTime = receiverTime.factory->makeInitial();
  require(
      queriedTime != nullptr,
      "TARA process requester could not allocate Query Logical Time value");
  receiver.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(
      *queriedTime,
      *requestBoundary,
      "TARA process requester final logical time");

  writeNmrProcessMarker(directory, "requester-update-received.marker");
  waitForNmrProcessMarker(directory, "publisher-resigned.marker", options);
  receiver.rtiAmbassador().disableTimeConstrained();
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      objectClass,
      attributes);
  receiver.resign(rti::NO_ACTION);
  receiver.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
}

void runTaraFutureTimestampedAttributeUpdatePublisherProcess(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& directory) {
  waitForNmrProcessMarker(directory, "federation-created.marker", options);
  Session publisher(options, model, "member");
  publisher.connect();
  auto const federation = federationName(
      options,
      "tara-future-timestamped-attribute-update");
  publisher.join(options.memberFederateName, options.federateType, federation);

  auto const objectClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = publisher.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "TARA process publisher could not resolve its object class and attribute");
  rti::AttributeHandleSet const attributes{attribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      objectClass,
      attributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      objectClass,
      attributes,
      rti::TIMESTAMP);

  auto publisherTime = makeTimeContext(publisher);
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      1U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "TARA process publisher could not allocate its lookahead");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "TARA process publisher Time Regulation callback");
  writeNmrProcessMarker(directory, "publisher-ready.marker");
  waitForNmrProcessMarker(directory, "requester-ready.marker", options);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(objectClass);
  require(object.isValid(), "TARA process publisher registered an invalid object");
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
      "TARA process publisher initial advance");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherCurrentTime),
      "TARA process publisher advanced to an unexpected initial time");
  writeNmrProcessMarker(directory, "publisher-ready-to-update.marker");
  waitForNmrProcessMarker(directory, "requester-pending.marker", options);

  auto const updateTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  std::vector<std::uint8_t> const valueBytes{'T', 'A', 'R', 'A', '5'};
  std::vector<std::uint8_t> const tagBytes{'T', 'A', 'R', 'A'};
  rti::AttributeHandleValueMap values;
  values.emplace(
      attribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *updateTime);
  require(
      retraction.isValid(),
      "TARA process publisher timestamped update had no retraction handle");
  writeNmrProcessMarker(directory, "publisher-update-sent.marker");
  waitForNmrProcessMarker(directory, "requester-update-queued.marker", options);

  auto const publisherAdvanceTarget = timeAfter(
      *publisherTime.factory,
      *publisherCurrentTime,
      *publisherTime.epsilon,
      4U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAdvanceTarget);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "TARA process publisher follow-up advance");
  require(
      publisher.recorder().timeAdvanceGrants().back().encoded ==
          encodeTime(*publisherAdvanceTarget),
      "TARA process publisher follow-up grant reached the wrong time");
  writeNmrProcessMarker(directory, "publisher-advanced.marker");
  waitForNmrProcessMarker(directory, "requester-update-received.marker", options);

  publisher.rtiAmbassador().disableTimeRegulation();
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      objectClass,
      attributes);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.disconnect();
  writeNmrProcessMarker(directory, "publisher-resigned.marker");
}

void runTaraFutureTimestampedAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "TARA future timestamped attribute update requires an adapter-supplied logical-time implementation");

  if (!options.nmrProcessRole.empty()) {
    auto const directory = nextNmrProcessRunDirectory(options);
    if (options.nmrProcessRole == "requester") {
      runTaraFutureTimestampedAttributeUpdateRequesterProcess(
          options,
          model,
          directory);
      return;
    }
    if (options.nmrProcessRole == "publisher") {
      runTaraFutureTimestampedAttributeUpdatePublisherProcess(
          options,
          model,
          directory);
      return;
    }
    throw std::runtime_error(
        "TARA process role must be either requester or publisher");
  }
  require(
      options.nmrProcessCoordinationDirectory.empty(),
      "TARA process coordination directory requires an explicit process role");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "tara-future-timestamped-attribute-update");

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
      "TARA future timestamped attribute update lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName() &&
          encodeTime(*publisherTime.initial) == encodeTime(*receiverTime.initial),
      "TARA future timestamped attribute update members selected incompatible logical times");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "TARA future timestamped attribute update time-constrained callback");

  auto const publisherLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      1U);
  auto publisherLookahead = publisherTime.factory->makeZero();
  require(
      publisherLookahead != nullptr,
      "TARA future timestamped attribute update could not allocate publisher lookahead");
  publisherLookahead->setToDifference(
      *publisherLookaheadTime,
      *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "TARA future timestamped attribute update time-regulation callback");

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
      "TARA future timestamped attribute update regulator advance");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherCurrentTime),
      "TARA future timestamped attribute update regulator advanced to the wrong time");

  auto const updateTime = timeAfter(
      *publisherTime.factory,
      *publisherCurrentTime,
      *publisherTime.epsilon,
      3U);
  auto const requestBoundary = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      6U);
  auto const publisherAdvanceTarget = timeAfter(
      *publisherTime.factory,
      *publisherCurrentTime,
      *publisherTime.epsilon,
      4U);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "TARA future timestamped attribute update registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "TARA future timestamped attribute update object discovery");

  receiver.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().timeAdvanceRequestAvailable(*requestBoundary);
  receiver.pump();
  require(
      receiver.recorder().timedReflections().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "TARA future timestamped attribute update completed before its future event arrived");

  std::vector<std::uint8_t> const valueBytes{'T', 'A', 'R', 'A', '5'};
  std::vector<std::uint8_t> const tagBytes{'T', 'A', 'R', 'A'};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *updateTime);
  require(
      retraction.isValid(),
      "TARA future timestamped attribute update returned an invalid retraction handle");
  require(
      receiver.recorder().timedReflections().empty(),
      "TARA future timestamped attribute update was delivered before its time became available");

  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAdvanceTarget);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "TARA future timestamped attribute update regulator follow-up advance");
  require(
      publisher.recorder().timeAdvanceGrants().back().encoded ==
          encodeTime(*publisherAdvanceTarget),
      "TARA future timestamped attribute update regulator follow-up grant was wrong");

  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedReflections().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "TARA future timestamped attribute update callbacks");

  auto const reflections = receiver.recorder().timedReflections();
  auto const grants = receiver.recorder().timeAdvanceGrants();
  require(
      reflections.size() == 1U && grants.size() == 1U,
      "TARA future timestamped attribute update returned duplicate or missing callbacks");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "TARA future timestamped attribute update granted before its future reflection");
  auto const& reflection = reflections.front();
  require(
      reflection.object == object && reflection.values.size() == 1U &&
          reflection.values.count(receiverAttribute) == 1U &&
          copyBytes(reflection.values.at(receiverAttribute)) == valueBytes &&
          reflection.tag == tagBytes &&
          reflection.producer == publisher.federateHandle() &&
          reflection.time == encodeTime(*updateTime) &&
          !reflection.timeText.empty() &&
          reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP &&
          reflection.retractionPresent &&
          reflection.retraction == copyBytes(retraction.encode()) &&
          reflection.transportation.isValid(),
      "TARA future timestamped attribute update reflection metadata was incorrect");
  require(
      grants.front().encoded == encodeTime(*requestBoundary),
      "TARA future timestamped attribute update returned the wrong grant time");
  auto queriedTime = receiverTime.factory->makeInitial();
  require(
      queriedTime != nullptr,
      "TARA future timestamped attribute update could not allocate queried logical time");
  receiver.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(
      *queriedTime,
      *requestBoundary,
      "TARA future timestamped attribute update logical-time query");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  receiver.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioTaraFutureTimestampedAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  runTaraFutureTimestampedAttributeUpdate(options, model);
}

void scenarioTaraFutureTimestampedAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  runTaraFutureTimestampedAttributeUpdate(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
