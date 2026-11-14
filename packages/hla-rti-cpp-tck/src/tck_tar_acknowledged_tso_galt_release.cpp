#include "tck_main_scenarios.hpp"

#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioTarAcknowledgedTsoGaltRelease(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Acknowledged-TSO TAR requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "acknowledged-tso-publisher");
  Session receiver(options, model, "acknowledged-tso-receiver");
  auto const federation = federationName(options, "tar-acknowledged-tso-galt-release");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid() &&
          publisherInteraction == receiverInteraction &&
          publisherParameter == receiverParameter,
      "Acknowledged-TSO TAR lookup returned inconsistent standard handles");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Acknowledged-TSO TAR members selected different logical-time factories");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "Acknowledged-TSO TAR constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Acknowledged-TSO TAR regulation callback");
  publisher.recorder().clearTimeCallbacks();
  receiver.recorder().clearTimeCallbacks();
  publisher.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      4U);
  std::vector<std::uint8_t> const valueBytes{0x54U, 0x41U, 0x52U};
  std::vector<std::uint8_t> const tagBytes{0x47U, 0x41U, 0x4CU, 0x54U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid() && receiver.recorder().timedInteractions().empty(),
      "Acknowledged-TSO TAR did not retain the timestamped interaction before advance");

  // Advance the regulator first: with one epsilon of lookahead its next send
  // frontier is later than receiverTarget. The receiver must deliver the
  // queued pre-target TSO before its safe TAR grant. Do not prescribe or query
  // a particular GALT formula; this case checks the public grant and callback.
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Acknowledged-TSO TAR regulator grant");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherTarget),
      "Acknowledged-TSO TAR regulator advanced to the wrong time");

  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  waitFor(
      publisher,
      receiver,
      [&] {
        return receiver.recorder().timedInteractions().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "Acknowledged-TSO TAR callback-before-grant sequence");
  require(
      receiver.recorder().timedInteractions().size() == 1U &&
          receiver.recorder().timeAdvanceGrants().size() == 1U &&
          receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "grant"},
      "Acknowledged-TSO TAR did not deliver exactly once before grant");
  auto const deliveredInteractions = receiver.recorder().timedInteractions();
  auto const& delivered = deliveredInteractions.front();
  auto const receivedParameter = delivered.parameters.find(receiverParameter);
  auto const receivedValueBytes = receivedParameter == delivered.parameters.end()
      ? std::vector<std::uint8_t>{}
      : copyBytes(receivedParameter->second);
  auto const formatBytes = [](std::vector<std::uint8_t> const& bytes) {
    std::string formatted{"["};
    for (std::size_t index = 0; index < bytes.size(); ++index) {
      if (index != 0U) {
        formatted += ",";
      }
      formatted += std::to_string(bytes[index]);
    }
    formatted += "]";
    return formatted;
  };
  auto const payloadMatches =
      delivered.interaction == receiverInteraction &&
      delivered.parameters.size() == 1U &&
      receivedParameter != delivered.parameters.end() &&
      receivedValueBytes == valueBytes && delivered.tag == tagBytes;
  require(
      payloadMatches,
      "Acknowledged-TSO TAR callback changed its public interaction value or tag; "
      "expected value=" + formatBytes(valueBytes) + " tag=" + formatBytes(tagBytes) +
      ", received parameter-count=" +
      std::to_string(delivered.parameters.size()) + " value=" +
      formatBytes(receivedValueBytes) + " tag=" + formatBytes(delivered.tag));
  require(
      delivered.time == encodeTime(*messageTime),
      "Acknowledged-TSO TAR callback changed its timestamp");
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*receiverTarget),
      "Acknowledged-TSO TAR granted at the wrong logical time");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTarMultiRegulatorAcknowledgedTsoProgress(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Multi-regulator TAR requires an adapter-supplied logical-time implementation");

  Session firstRegulator(options, model, "owner");
  Session secondRegulator(options, model, "member");
  Session receiver(options, model, "member-receiver");
  auto const federation = federationName(
      options, "tar-multi-regulator-acknowledged-tso-progress");
  connectAndJoin(firstRegulator, secondRegulator, options, federation, options.fom);
  receiver.connect();
  receiver.join(
      options.memberFederateName + L"-tar-receiver",
      options.federateType,
      federation);

  auto const firstInteraction =
      firstRegulator.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const secondInteraction =
      secondRegulator.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const firstParameter = firstRegulator.rtiAmbassador().getParameterHandle(
      firstInteraction, options.parameterName);
  auto const secondParameter = secondRegulator.rtiAmbassador().getParameterHandle(
      secondInteraction, options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction, options.parameterName);
  require(
      firstInteraction.isValid() && secondInteraction.isValid() &&
          receiverInteraction.isValid() && firstParameter.isValid() &&
          secondParameter.isValid() && receiverParameter.isValid() &&
          firstInteraction == secondInteraction &&
          firstInteraction == receiverInteraction &&
          firstParameter == secondParameter &&
          firstParameter == receiverParameter,
      "Multi-regulator TAR lookup returned inconsistent standard handles");

  firstRegulator.rtiAmbassador().publishInteractionClass(firstInteraction);
  firstRegulator.rtiAmbassador().changeInteractionOrderType(
      firstInteraction, rti::TIMESTAMP);
  secondRegulator.rtiAmbassador().publishInteractionClass(secondInteraction);
  secondRegulator.rtiAmbassador().changeInteractionOrderType(
      secondInteraction, rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto firstTime = makeTimeContext(firstRegulator);
  auto secondTime = makeTimeContext(secondRegulator);
  auto receiverTime = makeTimeContext(receiver);
  require(
      firstTime.factory->getName() == secondTime.factory->getName() &&
          firstTime.factory->getName() == receiverTime.factory->getName(),
      "Multi-regulator TAR members selected different logical-time factories");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "Multi-regulator TAR constrained callback");
  firstRegulator.rtiAmbassador().enableTimeRegulation(*firstTime.epsilon);
  waitFor(
      firstRegulator,
      [&] {
        return firstRegulator.recorder().timeRegulationEnabled().size() == 1U;
      },
      options,
      "First multi-regulator TAR regulation callback");
  secondRegulator.rtiAmbassador().enableTimeRegulation(*secondTime.epsilon);
  waitFor(
      secondRegulator,
      [&] {
        return secondRegulator.recorder().timeRegulationEnabled().size() == 1U;
      },
      options,
      "Second multi-regulator TAR regulation callback");

  firstRegulator.recorder().clearTimeCallbacks();
  secondRegulator.recorder().clearTimeCallbacks();
  receiver.recorder().clearTimeCallbacks();
  firstRegulator.recorder().clearCallbackOrder();
  secondRegulator.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();

  auto const firstMessageTime = timeAfter(
      *firstTime.factory, *firstTime.initial, *firstTime.epsilon, 3U);
  auto const secondMessageTime = timeAfter(
      *secondTime.factory, *secondTime.initial, *secondTime.epsilon, 5U);
  auto const firstRegulatorTarget = timeAfter(
      *firstTime.factory, *firstTime.initial, *firstTime.epsilon, 7U);
  auto const secondRegulatorTarget = timeAfter(
      *secondTime.factory, *secondTime.initial, *secondTime.epsilon, 8U);
  auto const receiverFirstTarget = timeAfter(
      *receiverTime.factory, *receiverTime.initial, *receiverTime.epsilon, 4U);
  auto const receiverSecondTarget = timeAfter(
      *receiverTime.factory, *receiverTime.initial, *receiverTime.epsilon, 6U);

  std::vector<std::uint8_t> const firstValue{0x41U, 0x33U};
  std::vector<std::uint8_t> const firstTag{0x54U, 0x53U, 0x4FU, 0x2DU, 0x41U};
  rti::ParameterHandleValueMap firstParameters;
  firstParameters.emplace(
      firstParameter,
      rti::VariableLengthData(firstValue.data(), firstValue.size()));
  rti::VariableLengthData firstTagData(firstTag.data(), firstTag.size());
  auto const firstRetraction = firstRegulator.rtiAmbassador().sendInteraction(
      firstInteraction, firstParameters, firstTagData, *firstMessageTime);

  std::vector<std::uint8_t> const secondValue{0x42U, 0x35U};
  std::vector<std::uint8_t> const secondTag{0x54U, 0x53U, 0x4FU, 0x2DU, 0x42U};
  rti::ParameterHandleValueMap secondParameters;
  secondParameters.emplace(
      secondParameter,
      rti::VariableLengthData(secondValue.data(), secondValue.size()));
  rti::VariableLengthData secondTagData(secondTag.data(), secondTag.size());
  auto const secondRetraction = secondRegulator.rtiAmbassador().sendInteraction(
      secondInteraction, secondParameters, secondTagData, *secondMessageTime);
  require(
      firstRetraction.isValid() && secondRetraction.isValid() &&
          receiver.recorder().timedInteractions().empty(),
      "Multi-regulator TAR did not queue both timestamped interactions");

  // Move both regulators beyond the two queued messages before requesting the
  // receiver's advances. This checks the public grant/delivery outcomes only:
  // it does not query GALT or prescribe an implementation-specific formula.
  firstRegulator.rtiAmbassador().timeAdvanceRequest(*firstRegulatorTarget);
  secondRegulator.rtiAmbassador().timeAdvanceRequest(*secondRegulatorTarget);
  std::vector<Session*> const regulators{&firstRegulator, &secondRegulator};
  waitForSessions(
      regulators,
      [&] {
        return firstRegulator.recorder().timeAdvanceGrants().size() == 1U &&
            secondRegulator.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "Both multi-regulator TAR regulator grants");
  require(
      firstRegulator.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*firstRegulatorTarget) &&
          secondRegulator.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*secondRegulatorTarget),
      "A multi-regulator TAR sender advanced to the wrong logical time");

  receiver.rtiAmbassador().timeAdvanceRequest(*receiverFirstTarget);
  std::vector<Session*> const allSessions{
      &firstRegulator, &secondRegulator, &receiver};
  waitForSessions(
      allSessions,
      [&] { return receiver.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Multi-regulator TAR(4) grant after queued TSO(3)");
  auto received = receiver.recorder().timedInteractions();
  auto grants = receiver.recorder().timeAdvanceGrants();
  require(
      received.size() == 1U && grants.size() == 1U &&
          receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "grant"},
      "Multi-regulator TAR(4) did not deliver only TSO(3) before its grant");
  require(
      received.front().interaction == receiverInteraction &&
          received.front().producer == firstRegulator.federateHandle() &&
          received.front().parameters.size() == 1U &&
          received.front().parameters.count(receiverParameter) == 1U &&
          copyBytes(received.front().parameters.at(receiverParameter)) ==
              firstValue &&
          received.front().tag == firstTag &&
          received.front().time == encodeTime(*firstMessageTime) &&
          received.front().sentOrder == rti::TIMESTAMP &&
          received.front().receivedOrder == rti::TIMESTAMP &&
          grants.front().encoded == encodeTime(*receiverFirstTarget),
      "Multi-regulator TAR(4) changed TSO(3) data or granted at the wrong time");

  // Returning from the callback above acknowledges TSO(3). The next TAR must
  // still make safe progress and deliver the remaining TSO(5) exactly once.
  receiver.recorder().clearCallbackOrder();
  require(
      receiver.recorder().timedInteractions().size() == 1U,
      "Multi-regulator TAR delivered a future TSO before the later request");
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverSecondTarget);
  waitForSessions(
      allSessions,
      [&] { return receiver.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "Multi-regulator TAR(6) progress after acknowledging TSO(3)");
  received = receiver.recorder().timedInteractions();
  grants = receiver.recorder().timeAdvanceGrants();
  require(
      received.size() == 2U && grants.size() == 2U &&
          receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "grant"},
      "Multi-regulator TAR(6) did not deliver the remaining TSO(5) once before grant");
  require(
      received.back().interaction == receiverInteraction &&
          received.back().producer == secondRegulator.federateHandle() &&
          received.back().parameters.size() == 1U &&
          received.back().parameters.count(receiverParameter) == 1U &&
          copyBytes(received.back().parameters.at(receiverParameter)) ==
              secondValue &&
          received.back().tag == secondTag &&
          received.back().time == encodeTime(*secondMessageTime) &&
          received.back().sentOrder == rti::TIMESTAMP &&
          received.back().receivedOrder == rti::TIMESTAMP &&
          grants.back().encoded == encodeTime(*receiverSecondTarget),
      "Multi-regulator TAR(6) changed TSO(5) data or granted at the wrong time");

  receiver.rtiAmbassador().disableTimeConstrained();
  firstRegulator.rtiAmbassador().disableTimeRegulation();
  secondRegulator.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  firstRegulator.rtiAmbassador().unpublishInteractionClass(firstInteraction);
  secondRegulator.rtiAmbassador().unpublishInteractionClass(secondInteraction);
  receiver.resign(rti::NO_ACTION);
  secondRegulator.resign(rti::NO_ACTION);
  firstRegulator.resign(rti::NO_ACTION);
  firstRegulator.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  secondRegulator.disconnect();
  firstRegulator.disconnect();
}

void scenarioTarAcknowledgedTsoGaltReleaseContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTarAcknowledgedTsoGaltRelease(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
