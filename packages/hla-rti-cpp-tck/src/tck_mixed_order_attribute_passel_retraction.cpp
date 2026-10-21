#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

namespace {

void runMixedOrderAttributePasselScenario(
    Options const& options,
    rti::CallbackModel model,
    bool retractTimestampedAttribute) {
  require(
      !options.multiAttributeFom.empty(),
      "Mixed-order passel scenarios require an adapter-supplied multi-attribute FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Mixed-order passel scenarios require an adapter-supplied logical-time implementation");

  Session publisher(options, model, "mixed-order-passel-publisher");
  Session receiver(options, model, "mixed-order-passel-receiver");
  auto const federation = federationName(
      options,
      retractTimestampedAttribute
          ? "mixed-order-attribute-passel-retraction"
          : "mixed-order-attribute-passels-separate-messages");
  connectAndJoin(
      publisher,
      receiver,
      options,
      federation,
      options.multiAttributeFom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const publisherTimestamped = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.multiAttributeFirstName);
  auto const publisherReceiveOrder = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.multiAttributeSecondName);
  auto const receiverTimestamped = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.multiAttributeFirstName);
  auto const receiverReceiveOrder = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.multiAttributeSecondName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherTimestamped.isValid() && publisherReceiveOrder.isValid() &&
          receiverTimestamped.isValid() && receiverReceiveOrder.isValid() &&
          publisherTimestamped != publisherReceiveOrder,
      "Mixed-order passel retraction resolved invalid or duplicate handles");

  rti::AttributeHandleSet const publisherAttributes{
      publisherTimestamped,
      publisherReceiveOrder};
  rti::AttributeHandleSet const receiverAttributes{
      receiverTimestamped,
      receiverReceiveOrder};
  rti::AttributeHandleSet const timestampedAttribute{publisherTimestamped};
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
      timestampedAttribute,
      rti::TIMESTAMP);

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "Mixed-order passel registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "Mixed-order passel discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Mixed-order passel members selected different logical-time factories");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Mixed-order passel time-constrained callback");
  receiver.rtiAmbassador().enableAsynchronousDelivery();
  auto const lookaheadTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Mixed-order passel could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTarget, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Mixed-order passel time-regulation callback");

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
  std::vector<std::uint8_t> const timestampedBytes{0x51U, 0x52U};
  std::vector<std::uint8_t> const receiveOrderBytes{0x61U, 0x62U, 0x63U};
  std::vector<std::uint8_t> const tagBytes{0x4DU, 0x49U, 0x58U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherTimestamped,
      rti::VariableLengthData(timestampedBytes.data(), timestampedBytes.size()));
  values.emplace(
      publisherReceiveOrder,
      rti::VariableLengthData(receiveOrderBytes.data(), receiveOrderBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *updateTime);
  require(
      retraction.isValid(),
      "Mixed-order passel update returned an invalid retraction handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timedReflections().size() >= 1U; },
      options,
      "Mixed-order passel receive-order reflection");
  auto const receivedBeforeRetraction = receiver.recorder().timedReflections();
  require(
      receivedBeforeRetraction.size() == 1U,
      "Mixed-order passel delivered an unexpected number of reflection callbacks before retraction");
  require(
      receivedBeforeRetraction.front().object == object &&
          receivedBeforeRetraction.front().values.size() == 1U &&
          receivedBeforeRetraction.front().values.count(receiverReceiveOrder) == 1U &&
          copyBytes(receivedBeforeRetraction.front().values.at(receiverReceiveOrder)) ==
              receiveOrderBytes &&
          receivedBeforeRetraction.front().receivedOrder == rti::RECEIVE &&
          !receivedBeforeRetraction.front().retractionPresent,
      "Mixed-order passel did not report only the receive-order attribute with receive-order metadata before retraction");

  if (retractTimestampedAttribute) {
    publisher.rtiAmbassador().retract(retraction);
  }
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      retractTimestampedAttribute
          ? "Mixed-order passel post-retraction grants"
          : "Mixed-order separate-message grants");
  if (retractTimestampedAttribute) {
    require(
        receiver.recorder().timedReflections().size() == 1U &&
            receiver.recorder().reflections().empty() &&
            receiver.recorder().timeAdvanceGrants().size() == 1U,
        "Mixed-order passel retraction changed the delivered receive-order result or released the queued timestamped attribute");
  } else {
    auto const deliveries = receiver.recorder().timedReflections();
    auto const receiveOrder = std::find_if(
        deliveries.begin(),
        deliveries.end(),
        [&](TimedReflectionRecord const& delivery) {
          return delivery.values.size() == 1U &&
              delivery.values.count(receiverReceiveOrder) == 1U;
        });
    auto const timestampOrder = std::find_if(
        deliveries.begin(),
        deliveries.end(),
        [&](TimedReflectionRecord const& delivery) {
          return delivery.values.size() == 1U &&
              delivery.values.count(receiverTimestamped) == 1U;
        });
    require(
        deliveries.size() == 2U && receiveOrder != deliveries.end() &&
            timestampOrder != deliveries.end() &&
            receiveOrder != timestampOrder &&
            receiveOrder->object == object &&
            copyBytes(receiveOrder->values.at(receiverReceiveOrder)) ==
                receiveOrderBytes &&
            receiveOrder->sentOrder == rti::RECEIVE &&
            receiveOrder->receivedOrder == rti::RECEIVE &&
            receiveOrder->tag == tagBytes &&
            !receiveOrder->retractionPresent &&
            timestampOrder->object == object &&
            copyBytes(timestampOrder->values.at(receiverTimestamped)) ==
                timestampedBytes &&
            timestampOrder->sentOrder == rti::TIMESTAMP &&
            timestampOrder->receivedOrder == rti::TIMESTAMP &&
            timestampOrder->time == encodeTime(*updateTime) &&
            timestampOrder->tag == tagBytes &&
            timestampOrder->retractionPresent &&
            receiver.recorder().reflections().empty() &&
            receiver.recorder().timeAdvanceGrants().size() == 1U,
        "A mixed-order timestamped update was not delivered as two distinct attribute-only passel messages");
  }

  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioMixedOrderAttributePasselRetraction(
    Options const& options,
    rti::CallbackModel model) {
  runMixedOrderAttributePasselScenario(options, model, true);
}

void scenarioMixedOrderAttributePasselRetractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioMixedOrderAttributePasselRetraction(options, model);
}

void scenarioMixedOrderAttributePasselsSeparateMessages(
    Options const& options,
    rti::CallbackModel model) {
  runMixedOrderAttributePasselScenario(options, model, false);
}

void scenarioMixedOrderAttributePasselsSeparateMessagesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioMixedOrderAttributePasselsSeparateMessages(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
