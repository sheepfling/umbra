#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimestampedAttributeImmediateRecipientMultiTransportRetraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.multiAttributeFom.empty(),
      "Immediate-recipient multi-transport retraction requires an adapter-supplied multi-attribute FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Immediate-recipient multi-transport retraction requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "immediate-retraction-publisher");
  Session receiver(options, model, "immediate-retraction-receiver");
  auto const federation = federationName(
      options,
      "timestamped-attribute-immediate-recipient-multi-transport-retraction");
  connectAndJoin(
      publisher,
      receiver,
      options,
      federation,
      options.multiAttributeFom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.timestampedAttributePasselObjectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.timestampedAttributePasselObjectClassName);
  auto const publisherReliableAttribute =
      publisher.rtiAmbassador().getAttributeHandle(
          publisherClass,
          options.timestampedAttributePasselReliableAttributeName);
  auto const publisherBestEffortAttribute =
      publisher.rtiAmbassador().getAttributeHandle(
          publisherClass,
          options.timestampedAttributePasselBestEffortAttributeName);
  auto const receiverReliableAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.timestampedAttributePasselReliableAttributeName);
  auto const receiverBestEffortAttribute =
      receiver.rtiAmbassador().getAttributeHandle(
          receiverClass,
      options.timestampedAttributePasselBestEffortAttributeName);
  auto const reliable = publisher.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  auto const bestEffort = publisher.rtiAmbassador().getTransportationTypeHandle(
      L"HLAbestEffort");
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherReliableAttribute.isValid() &&
          publisherBestEffortAttribute.isValid() &&
          receiverReliableAttribute.isValid() &&
          receiverBestEffortAttribute.isValid() &&
          publisherReliableAttribute != publisherBestEffortAttribute &&
          reliable.isValid() && bestEffort.isValid() && reliable != bestEffort,
      "Immediate-recipient multi-transport retraction resolved invalid or duplicate handles");

  rti::AttributeHandleSet const publisherAttributes{
      publisherReliableAttribute,
      publisherBestEffortAttribute};
  rti::AttributeHandleSet const receiverAttributes{
      receiverReliableAttribute,
      receiverBestEffortAttribute};
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
      "Immediate-recipient multi-transport registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "Immediate-recipient multi-transport discovery");

  auto const publisherTime = makeTimeContext(publisher);
  auto const lookaheadTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Immediate-recipient multi-transport retraction could not allocate lookahead");
  lookahead->setToDifference(*lookaheadTarget, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Immediate-recipient multi-transport time-regulation callback");

  auto const updateTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  std::vector<std::uint8_t> const reliableBytes{0x52U, 0x45U, 0x4CU};
  std::vector<std::uint8_t> const bestEffortBytes{0x42U, 0x45U};
  std::vector<std::uint8_t> const tagBytes{0x4DU, 0x54U, 0x53U, 0x4FU};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherReliableAttribute,
      rti::VariableLengthData(reliableBytes.data(), reliableBytes.size()));
  values.emplace(
      publisherBestEffortAttribute,
      rti::VariableLengthData(bestEffortBytes.data(), bestEffortBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *updateTime);
  require(
      retraction.isValid(),
      "Immediate-recipient multi-transport update returned an invalid retraction handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timedReflections().size() >= 2U; },
      options,
      "Immediate-recipient multi-transport reflections");

  auto const reflections = receiver.recorder().timedReflections();
  require(
      reflections.size() == 2U,
      "Immediate-recipient multi-transport update did not produce exactly two passel callbacks");
  bool reliablePasselSeen = false;
  bool bestEffortPasselSeen = false;
  for (auto const& reflection : reflections) {
    require(
        reflection.object == object && reflection.values.size() == 1U &&
            reflection.tag == tagBytes &&
            reflection.time == encodeTime(*updateTime) &&
            !reflection.timeText.empty() &&
            reflection.producer == publisher.federateHandle() &&
            reflection.sentOrder == rti::TIMESTAMP &&
            reflection.receivedOrder == rti::RECEIVE &&
            reflection.retractionPresent &&
            reflection.retraction == copyBytes(retraction.encode()),
        "Immediate-recipient multi-transport reflection returned incorrect update metadata");

    auto const reliableValue = reflection.values.find(receiverReliableAttribute);
    if (reliableValue != reflection.values.end()) {
      require(
          !reliablePasselSeen &&
              copyBytes(reliableValue->second) == reliableBytes &&
              receiver.rtiAmbassador().getTransportationTypeName(
                  reflection.transportation) == L"HLAreliable",
          "Immediate-recipient reliable passel returned the wrong value or transportation");
      reliablePasselSeen = true;
      continue;
    }

    auto const bestEffortValue =
        reflection.values.find(receiverBestEffortAttribute);
    require(
        bestEffortValue != reflection.values.end() && !bestEffortPasselSeen &&
            copyBytes(bestEffortValue->second) == bestEffortBytes &&
            receiver.rtiAmbassador().getTransportationTypeName(
                reflection.transportation) == L"HLAbestEffort",
        "Immediate-recipient best-effort passel returned the wrong value or transportation");
    bestEffortPasselSeen = true;
  }
  require(
      reliablePasselSeen && bestEffortPasselSeen,
      "Immediate-recipient multi-transport update omitted one transportation passel");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "reflect"},
      "Immediate-recipient multi-transport update returned the wrong reflection callback count");

  publisher.rtiAmbassador().retract(retraction);
  waitFor(
      receiver,
      [&] { return receiver.recorder().retractions().size() >= 1U; },
      options,
      "Immediate-recipient multi-transport Request Retraction callback");
  auto const retractions = receiver.recorder().retractions();
  require(
      retractions.size() == 1U && retractions.front().valid &&
          retractions.front().encoded == copyBytes(retraction.encode()),
      "Immediate-recipient multi-transport update returned the wrong retraction callback");
  require(
      receiver.recorder().timedReflections().size() == 2U &&
          receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"reflect", "reflect", "request-retraction"},
      "Immediate-recipient multi-transport retraction changed delivered callbacks");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "Immediate-recipient multi-transport terminal retraction");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedAttributeImmediateRecipientMultiTransportRetractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeImmediateRecipientMultiTransportRetraction(
      options,
      model);
}

}  // namespace hla_rti_cpp_tck::detail
