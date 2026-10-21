#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimestampedAttributeSenderRegulationTransition(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped sender-role transition requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "timestamped-sender-role-publisher");
  Session receiver(options, model, "timestamped-sender-role-receiver");
  auto const federation = federationName(
      options,
      "timestamped-attribute-sender-regulation-transition");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "Timestamped sender-role transition resolved an invalid handle");

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
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "Timestamped sender-role transition returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "Timestamped sender-role transition discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Timestamped sender-role transition selected different logical-time factories");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Timestamped sender-role transition time-constrained callback");
  receiver.rtiAmbassador().enableAsynchronousDelivery();

  auto const lookaheadTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Timestamped sender-role transition could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTarget, *publisherTime.initial);
  auto const updateTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      6U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);

  std::vector<std::uint8_t> const firstValueBytes{0x41U, 0x42U};
  std::vector<std::uint8_t> const firstTagBytes{0x31U};
  rti::AttributeHandleValueMap firstValues;
  firstValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(firstValueBytes.data(), firstValueBytes.size()));
  rti::VariableLengthData const firstTag(firstTagBytes.data(), firstTagBytes.size());
  auto const firstRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      firstValues,
      firstTag,
      *updateTime);
  require(
      !firstRetraction.isValid(),
      "Timestamped send before Time Regulation returned a valid retraction handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timedReflections().size() >= 1U; },
      options,
      "Timestamped sender-role transition initial receive-order reflection");
  auto const firstReflections = receiver.recorder().timedReflections();
  require(
      firstReflections.size() == 1U &&
          firstReflections.front().object == object &&
          firstReflections.front().values.size() == 1U &&
          firstReflections.front().values.count(receiverAttribute) == 1U &&
          copyBytes(firstReflections.front().values.at(receiverAttribute)) ==
              firstValueBytes &&
          firstReflections.front().tag == firstTagBytes &&
          firstReflections.front().time == encodeTime(*updateTime) &&
          firstReflections.front().sentOrder == rti::RECEIVE &&
          firstReflections.front().receivedOrder == rti::RECEIVE &&
          !firstReflections.front().retractionPresent,
      "Timestamped send before Time Regulation did not arrive as receive-order");
  require(
      receiver.recorder().timeAdvanceGrants().empty(),
      "Timestamped sender-role transition granted receiver time before a request");

  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped sender-role transition time-regulation callback");
  auto const regulationEnabledReports =
      publisher.recorder().timeRegulationEnabled();
  auto assignedPublisherTime = publisherTime.factory->makeInitial();
  require(
      assignedPublisherTime != nullptr,
      "Timestamped sender-role transition could not allocate an assigned-time query target");
  publisher.rtiAmbassador().queryLogicalTime(*assignedPublisherTime);
  require(
      regulationEnabledReports.size() == 1U &&
          regulationEnabledReports.front().encoded ==
              encodeTime(*publisherTime.initial) &&
          encodeTime(*assignedPublisherTime) ==
              regulationEnabledReports.front().encoded,
      "Time Regulation Enabled did not report the assigned initial logical time");
  auto assignedLookahead = publisherTime.factory->makeZero();
  require(
      assignedLookahead != nullptr,
      "Timestamped sender-role transition could not allocate a lookahead query target");
  publisher.rtiAmbassador().queryLookahead(*assignedLookahead);
  require(
      copyBytes(assignedLookahead->encode()) == copyBytes(lookahead->encode()),
      "Enable Time Regulation did not retain the requested lookahead");

  std::vector<std::uint8_t> const secondValueBytes{0x54U, 0x53U, 0x4FU};
  std::vector<std::uint8_t> const secondTagBytes{0x32U};
  rti::AttributeHandleValueMap secondValues;
  secondValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(secondValueBytes.data(), secondValueBytes.size()));
  rti::VariableLengthData const secondTag(secondTagBytes.data(), secondTagBytes.size());
  auto const secondRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      secondValues,
      secondTag,
      *updateTime);
  require(
      secondRetraction.isValid(),
      "Timestamped send after Time Regulation returned an invalid retraction handle");
  require(
      receiver.recorder().timedReflections().size() == 1U &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "Timestamped send after Time Regulation was delivered before receiver advancement");

  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return receiver.recorder().timedReflections().size() >= 2U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            publisher.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "Timestamped sender-role transition TSO delivery and grant");
  auto const reflections = receiver.recorder().timedReflections();
  require(
      reflections.size() == 2U &&
          receiver.recorder().timeAdvanceGrants().size() == 1U &&
          receiver.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*receiverTarget),
      "Timestamped sender-role transition returned an unexpected reflection or grant count");
  auto const& second = reflections.back();
  require(
      second.object == object && second.values.size() == 1U &&
          second.values.count(receiverAttribute) == 1U &&
          copyBytes(second.values.at(receiverAttribute)) == secondValueBytes &&
          second.tag == secondTagBytes && second.time == encodeTime(*updateTime) &&
          second.sentOrder == rti::TIMESTAMP &&
          second.receivedOrder == rti::TIMESTAMP &&
          second.retractionPresent &&
          second.retraction == copyBytes(secondRetraction.encode()),
      "Timestamped send after Time Regulation returned incorrect timestamp-order metadata");
  auto const callbackOrder = receiver.recorder().callbackOrder();
  require(
      callbackOrder.size() >= 3U && callbackOrder.front() == "reflect" &&
          callbackOrder.back() == "grant" &&
          std::count(callbackOrder.begin(), callbackOrder.end(), "reflect") == 2,
      "Timestamped sender-role transition returned an unexpected callback order");

  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedAttributeSenderRegulationTransitionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeSenderRegulationTransition(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
