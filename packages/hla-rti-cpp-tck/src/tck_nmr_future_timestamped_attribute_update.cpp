#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runNmrFutureTimestampedAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "NMR future timestamped attribute update requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "nmr-future-timestamped-attribute-update");

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
      "NMR future attribute update lookup returned an invalid handle");

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
      "NMR future attribute update members selected incompatible logical times");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMR future attribute update time-constrained callback");

  auto const publisherLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto publisherLookahead = publisherTime.factory->makeZero();
  require(publisherLookahead != nullptr, "NMR future attribute update lookahead allocation failed");
  publisherLookahead->setToDifference(
      *publisherLookaheadTime,
      *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR future attribute update time-regulation callback");

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
      "NMR future attribute update regulator advance");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherCurrentTime),
      "NMR future attribute update regulator advanced to the wrong time");

  auto const updateTime = timeAfter(
      *publisherTime.factory,
      *publisherCurrentTime,
      *publisherTime.epsilon,
      5U);
  auto const requestBoundary = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      10U);
  auto actualGalt = receiverTime.factory->makeInitial();
  require(
      actualGalt != nullptr && receiver.rtiAmbassador().queryGALT(*actualGalt),
      "NMR future attribute update did not expose a defined GALT");
  requireTimeEquals(
      *actualGalt,
      *updateTime,
      "NMR future attribute update pre-request GALT");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(object.isValid(), "NMR future attribute registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "NMR future attribute update discovery");

  receiver.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().nextMessageRequest(*requestBoundary);
  receiver.pump();
  require(
      receiver.recorder().timedReflections().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "NMR completed before the future timestamped attribute update arrived");

  std::vector<std::uint8_t> const valueBytes{'N', 'M', 'R', '7'};
  std::vector<std::uint8_t> const tagBytes{'N', 'M', 'R'};
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
  require(retraction.isValid(), "NMR future attribute update returned an invalid retraction handle");

  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedReflections().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "NMR future attribute update callbacks");

  auto const reflections = receiver.recorder().timedReflections();
  auto const grants = receiver.recorder().timeAdvanceGrants();
  require(
      reflections.size() == 1U && grants.size() == 1U,
      "NMR future attribute update returned duplicate or missing callbacks");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "NMR future attribute update granted before its reflection");
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
      "NMR future attribute update reflection metadata was incorrect");
  require(
      grants.front().encoded == encodeTime(*updateTime),
      "NMR future attribute update returned the wrong grant time");
  auto queriedTime = receiverTime.factory->makeInitial();
  require(queriedTime != nullptr, "NMR future attribute query-time allocation failed");
  receiver.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(
      *queriedTime,
      *updateTime,
      "NMR future attribute update logical-time query");

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

void scenarioNmrFutureTimestampedAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  runNmrFutureTimestampedAttributeUpdate(options, model);
}

void scenarioNmrFutureTimestampedAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  runNmrFutureTimestampedAttributeUpdate(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
