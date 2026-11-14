#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioInteractionMultiRecipientFifo(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "ordinary-interaction-fanout-publisher");
  Session first(options, model, "ordinary-interaction-fanout-first");
  Session second(options, model, "ordinary-interaction-fanout-second");
  auto const federation = federationName(
      options,
      "ordinary-interaction-multi-recipient-fifo");
  connectAndJoin(publisher, first, options, federation, options.fom);
  second.connect();
  second.join(
      options.memberFederateName + L"-second",
      options.federateType,
      federation);

  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const firstInteraction =
      first.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const secondInteraction =
      second.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const firstParameter = first.rtiAmbassador().getParameterHandle(
      firstInteraction,
      options.parameterName);
  auto const secondParameter = second.rtiAmbassador().getParameterHandle(
      secondInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && firstInteraction.isValid() &&
          secondInteraction.isValid() && publisherParameter.isValid() &&
          firstParameter.isValid() && secondParameter.isValid() &&
          publisher.rtiAmbassador().getInteractionClassName(publisherInteraction) ==
              options.interactionClassName &&
          first.rtiAmbassador().getInteractionClassName(firstInteraction) ==
              options.interactionClassName &&
          second.rtiAmbassador().getInteractionClassName(secondInteraction) ==
              options.interactionClassName &&
          publisherInteraction == firstInteraction &&
          firstInteraction == secondInteraction &&
          publisherParameter == firstParameter && firstParameter == secondParameter,
      "FOM interaction name or handle identity differed across members");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  first.rtiAmbassador().subscribeInteractionClass(firstInteraction, true);
  second.rtiAmbassador().subscribeInteractionClass(secondInteraction, true);
  publisher.recorder().clearInteraction();
  first.recorder().clearInteraction();
  second.recorder().clearInteraction();

  auto send = [&](std::vector<std::uint8_t> const& value,
                  std::vector<std::uint8_t> const& tagBytes) {
    rti::ParameterHandleValueMap parameters;
    parameters.emplace(
        publisherParameter,
        rti::VariableLengthData(value.data(), value.size()));
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        parameters,
        tag);
  };

  std::vector<std::uint8_t> const firstValue{
      0x46U, 0x49U, 0x52U, 0x53U, 0x54U};
  std::vector<std::uint8_t> const firstTag{0x31U};
  std::vector<std::uint8_t> const secondValue{
      0x53U, 0x45U, 0x43U, 0x4FU, 0x4EU, 0x44U};
  std::vector<std::uint8_t> const secondTag{0x32U};
  send(firstValue, firstTag);
  send(secondValue, secondTag);

  waitForSessions(
      {&first, &second},
      [&] {
        return first.recorder().interactions().size() >= 2U &&
            second.recorder().interactions().size() >= 2U;
      },
      options,
      "ordinary interaction multi-recipient FIFO delivery");
  require(
      publisher.recorder().interactions().empty(),
      "ordinary interaction multi-recipient FIFO delivered an interaction back to its sender");

  auto assertDelivery = [&](Session& receiver,
                            rti::InteractionClassHandle const& interaction,
                            rti::ParameterHandle const& parameter,
                            std::string const& description) {
    auto const received = receiver.recorder().interactions();
    require(
        received.size() == 2U,
        description + " returned an unexpected callback count");
    auto assertRecord = [&](InteractionRecord const& record,
                            std::vector<std::uint8_t> const& expectedValue,
                            std::vector<std::uint8_t> const& expectedTag,
                            std::string const& orderDescription) {
      require(
          record.interaction == interaction && record.parameters.size() == 1U &&
              record.parameters.count(parameter) == 1U &&
              copyBytes(record.parameters.at(parameter)) == expectedValue,
          orderDescription + " returned the wrong interaction or parameter value");
      require(
          record.tag == expectedTag,
          orderDescription + " returned the wrong user tag");
      require(
          record.producer == publisher.federateHandle(),
          orderDescription + " returned the wrong producing federate");
      require(
          record.transportation.isValid() &&
              !receiver.rtiAmbassador()
                   .getTransportationTypeName(record.transportation)
                   .empty(),
          orderDescription + " returned an unknown transportation type");
    };
    assertRecord(
        received.front(),
        firstValue,
        firstTag,
        description + " first receive-order callback");
    assertRecord(
        received.back(),
        secondValue,
        secondTag,
        description + " second receive-order callback");
  };
  assertDelivery(
      first,
      firstInteraction,
      firstParameter,
      "first ordinary interaction recipient");
  assertDelivery(
      second,
      secondInteraction,
      secondParameter,
      "second ordinary interaction recipient");

  first.rtiAmbassador().unsubscribeInteractionClass(firstInteraction);
  second.rtiAmbassador().unsubscribeInteractionClass(secondInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  second.resign(rti::NO_ACTION);
  first.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  second.disconnect();
  first.disconnect();
  publisher.disconnect();
}

void scenarioInteractionMultiRecipientFifoContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioInteractionMultiRecipientFifo(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
