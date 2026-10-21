#include "tck_main_scenarios.hpp"

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
  auto const& delivered = receiver.recorder().timedInteractions().front();
  require(
      delivered.interaction == receiverInteraction &&
          delivered.parameters.size() == 1U &&
          delivered.parameters.count(receiverParameter) == 1U &&
          copyBytes(delivered.parameters.at(receiverParameter)) == valueBytes &&
          delivered.tag == tagBytes,
      "Acknowledged-TSO TAR callback changed its public interaction value or tag");
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

void scenarioTarAcknowledgedTsoGaltReleaseContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTarAcknowledgedTsoGaltRelease(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
