#include "tck_harness.hpp"
#include "tck_nmr_process_coordination.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

void requireNmraAttributeUpdatePending(
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

void runNmraFutureTimestampedAttributeUpdateRequesterProcess(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& directory) {
  Session receiver(options, model, "owner");
  receiver.connect();
  auto const federation = federationName(
      options,
      "nmra-future-timestamped-attribute-update");
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
      "NMRA process requester could not resolve its object class and attribute");
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
      "NMRA process requester Time Constrained callback");
  writeNmrProcessMarker(directory, "requester-ready.marker");
  waitForNmrProcessMarker(directory, "publisher-ready.marker", options);
  waitForNmrProcessMarker(directory, "publisher-object-registered.marker", options);
  waitFor(
      receiver,
      [&] { return receiver.recorder().discoveryCount() == 1U; },
      options,
      "independent publisher's NMRA attribute-update object discovery");
  waitForNmrProcessMarker(directory, "publisher-ready-to-send.marker", options);

  auto const requestBoundary = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      10U);
  auto const updateTime = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      8U);
  receiver.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  requireNmraAttributeUpdatePending(
      receiver,
      "NMRA process requester completed before the publisher sent its update");
  writeNmrProcessMarker(directory, "requester-pending.marker");

  waitForNmrProcessMarker(directory, "publisher-update-sent.marker", options);
  requireNmraAttributeUpdatePending(
      receiver,
      "NMRA process requester delivered the update before the publisher advanced");
  writeNmrProcessMarker(directory, "requester-update-queued.marker");

  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedReflections().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "independent publisher's NMRA timestamped attribute update");
  auto const reflection = receiver.recorder().timedReflections().front();
  auto const discovery = receiver.recorder().discovery();
  std::vector<std::uint8_t> const valueBytes{'N', 'M', 'R', 'A'};
  std::vector<std::uint8_t> const tagBytes{'N', 'M', 'R', 'A'};
  require(
      discovery.present && discovery.objectClass == objectClass &&
          reflection.object == discovery.object &&
          reflection.values.size() == 1U &&
          reflection.values.count(attribute) == 1U &&
          copyBytes(reflection.values.at(attribute)) == valueBytes &&
          reflection.tag == tagBytes &&
          reflection.producer == discovery.producer &&
          reflection.time == encodeTime(*updateTime) &&
          !reflection.timeText.empty() &&
          reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP &&
          reflection.retractionPresent && !reflection.retraction.empty() &&
          reflection.transportation.isValid(),
      "NMRA process requester received incorrect timestamped reflection metadata");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "NMRA process requester grant after publisher advances");
  auto const grants = receiver.recorder().timeAdvanceGrants();
  require(
      grants.front().encoded == encodeTime(*updateTime) &&
          receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"reflect", "grant"},
      "NMRA process requester did not grant at the delivered update time after reflection");

  auto queriedTime = receiverTime.factory->makeInitial();
  require(
      queriedTime != nullptr,
      "NMRA process requester could not allocate Query Logical Time value");
  receiver.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(
      *queriedTime,
      *updateTime,
      "NMRA process requester final logical time");

  waitForNmrProcessMarker(directory, "publisher-resigned.marker", options);
  receiver.rtiAmbassador().disableTimeConstrained();
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      objectClass,
      attributes);
  receiver.resign(rti::NO_ACTION);
  receiver.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
}

void runNmraFutureTimestampedAttributeUpdatePublisherProcess(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& directory) {
  waitForNmrProcessMarker(directory, "federation-created.marker", options);
  Session publisher(options, model, "member");
  publisher.connect();
  auto const federation = federationName(
      options,
      "nmra-future-timestamped-attribute-update");
  publisher.join(options.memberFederateName, options.federateType, federation);

  auto const objectClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = publisher.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "NMRA process publisher could not resolve its object class and attribute");
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
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "NMRA process publisher could not allocate its lookahead");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMRA process publisher Time Regulation callback");
  writeNmrProcessMarker(directory, "publisher-ready.marker");
  waitForNmrProcessMarker(directory, "requester-ready.marker", options);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(objectClass);
  require(object.isValid(), "NMRA process publisher registered an invalid object");
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
      "NMRA process publisher initial advance");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherCurrentTime),
      "NMRA process publisher advanced to an unexpected initial time");
  writeNmrProcessMarker(directory, "publisher-ready-to-send.marker");
  waitForNmrProcessMarker(directory, "requester-pending.marker", options);

  auto const updateTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      8U);
  std::vector<std::uint8_t> const valueBytes{'N', 'M', 'R', 'A'};
  std::vector<std::uint8_t> const tagBytes{'N', 'M', 'R', 'A'};
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
      "NMRA process publisher timestamped update had no retraction handle");
  writeNmrProcessMarker(directory, "publisher-update-sent.marker");
  waitForNmrProcessMarker(directory, "requester-update-queued.marker", options);

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
      "NMRA process publisher final advance");
  require(
      publisher.recorder().timeAdvanceGrants().back().encoded ==
          encodeTime(*publisherAdvance),
      "NMRA process publisher final advance reached the wrong time");

  publisher.rtiAmbassador().disableTimeRegulation();
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      objectClass,
      attributes);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.disconnect();
  writeNmrProcessMarker(directory, "publisher-resigned.marker");
}

void runNmraFutureTimestampedAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "NMRA future timestamped attribute update requires an adapter-supplied logical-time implementation");

  if (!options.nmrProcessRole.empty()) {
    auto const directory = nextNmrProcessRunDirectory(options);
    if (options.nmrProcessRole == "requester") {
      runNmraFutureTimestampedAttributeUpdateRequesterProcess(
          options,
          model,
          directory);
      return;
    }
    if (options.nmrProcessRole == "publisher") {
      runNmraFutureTimestampedAttributeUpdatePublisherProcess(
          options,
          model,
          directory);
      return;
    }
    throw std::runtime_error(
        "NMRA process role must be either requester or publisher");
  }
  require(
      options.nmrProcessCoordinationDirectory.empty(),
      "NMRA process coordination directory requires an explicit process role");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "nmra-future-timestamped-attribute-update");
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
      "NMRA future attribute update lookup returned an invalid handle");
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
      "NMRA future attribute update members selected incompatible logical times");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMRA future attribute update time-constrained callback");
  auto const publisherLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto publisherLookahead = publisherTime.factory->makeZero();
  require(
      publisherLookahead != nullptr,
      "NMRA future attribute update lookahead allocation failed");
  publisherLookahead->setToDifference(
      *publisherLookaheadTime,
      *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMRA future attribute update time-regulation callback");

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
      "NMRA future attribute update regulator advance");
  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "NMRA future attribute registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "NMRA future attribute update discovery");

  auto const updateTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      8U);
  auto const requestBoundary = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      10U);
  receiver.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  requireNmraAttributeUpdatePending(
      receiver,
      "NMRA completed before the future timestamped attribute update arrived");

  std::vector<std::uint8_t> const valueBytes{'N', 'M', 'R', 'A'};
  std::vector<std::uint8_t> const tagBytes{'N', 'M', 'R', 'A'};
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
      "NMRA future attribute update returned an invalid retraction handle");
  requireNmraAttributeUpdatePending(
      receiver,
      "NMRA delivered an update before the source advanced to its timestamp");

  auto const publisherAdvance = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAdvance);
  waitForSessions(
      {&receiver, &publisher},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() == 2U &&
            receiver.recorder().timedReflections().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "NMRA future attribute update reflection, grant, and regulator advance");
  auto const reflection = receiver.recorder().timedReflections().front();
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*updateTime) &&
          receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"reflect", "grant"},
      "NMRA future attribute update grant did not follow reflection at the message time");
  require(
          reflection.object == object &&
          reflection.values.size() == 1U &&
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
      "NMRA future attribute update reflection metadata was incorrect");

  auto queriedTime = receiverTime.factory->makeInitial();
  require(
      queriedTime != nullptr,
      "NMRA future attribute query-time allocation failed");
  receiver.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(
      *queriedTime,
      *updateTime,
      "NMRA future attribute update logical-time query");

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

void scenarioNmraFutureTimestampedAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  runNmraFutureTimestampedAttributeUpdate(options, model);
}

void scenarioNmraFutureTimestampedAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  runNmraFutureTimestampedAttributeUpdate(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
