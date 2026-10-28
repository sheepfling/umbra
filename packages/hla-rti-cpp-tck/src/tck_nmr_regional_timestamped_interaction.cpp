#include "tck_nmr_regional_timestamped_interaction.hpp"

#include "tck_harness.hpp"

#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioNmrRegionalTimestampedInteractionMessageSet(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.ddmFom.empty(),
          "Regional NMR interaction requires the adapter-supplied DDM FOM");
  require(!options.ddmDimensionNames.empty(),
          "Regional NMR interaction requires an adapter-supplied DDM dimension");

  auto regionalOptions = options;
  regionalOptions.fom = options.ddmFom;
  Session publisher(regionalOptions, model, "regional-nmr-interaction-publisher");
  Session receiver(regionalOptions, model, "regional-nmr-interaction-receiver");
  auto const federation = federationName(
      regionalOptions,
      "nmr-regional-timestamped-interaction-message-set");
  connectAndJoin(
      publisher,
      receiver,
      regionalOptions,
      federation,
      regionalOptions.fom);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      regionalOptions.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      regionalOptions.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      regionalOptions.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      regionalOptions.parameterName);
  auto const publisherDimension = publisher.rtiAmbassador().getDimensionHandle(
      regionalOptions.ddmDimensionNames.front());
  auto const receiverDimension = receiver.rtiAmbassador().getDimensionHandle(
      regionalOptions.ddmDimensionNames.front());
  require(
      publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid() &&
          publisherDimension.isValid() && receiverDimension.isValid(),
      "Regional NMR interaction resolved an invalid FOM handle");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  auto const publisherRegion = publisher.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{publisherDimension});
  publisher.rtiAmbassador().setRangeBounds(
      publisherRegion,
      publisherDimension,
      rti::RangeBounds(0UL, 6UL));
  publisher.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{publisherRegion});

  auto const receiverRegion = receiver.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{receiverDimension});
  receiver.rtiAmbassador().setRangeBounds(
      receiverRegion,
      receiverDimension,
      rti::RangeBounds(5UL, 10UL));
  receiver.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{receiverRegion});
  receiver.rtiAmbassador().subscribeInteractionClassWithRegions(
      receiverInteraction,
      rti::RegionHandleSet{receiverRegion});

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Regional NMR participants selected different logical-time factories");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      regionalOptions,
      "regional NMR time-constrained enablement");

  auto lookahead = publisherTime.factory->makeZero();
  require(lookahead != nullptr,
          "Logical-time factory returned no regional NMR lookahead interval");
  for (unsigned step = 0U; step != 5U; ++step) {
    *lookahead += *publisherTime.epsilon;
  }
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      regionalOptions,
      "regional NMR publisher time-regulation enablement");

  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto const timestamp = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      7U);
  auto const receiverRequest = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      10U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      regionalOptions,
      "regional NMR publisher grant at its lookahead frontier");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherTarget),
      "Regional NMR publisher did not reach its requested lookahead frontier");

  std::vector<std::uint8_t> const parameterBytes{0x52U, 0x4EU, 0x4DU};
  std::vector<std::uint8_t> const tagBytes{0x52U, 0x54U, 0x53U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteractionWithRegions(
      publisherInteraction,
      parameters,
      rti::RegionHandleSet{publisherRegion},
      tag,
      *timestamp);
  require(retraction.isValid(),
          "Regional NMR timestamped interaction returned an invalid retraction handle");
  require(
      receiver.recorder().timedInteractions().empty(),
      "Regional NMR timestamped interaction arrived before its request");

  receiver.rtiAmbassador().nextMessageRequest(*receiverRequest);
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedInteractions().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 1U;
      },
      regionalOptions,
      "regional NMR message-set interaction and grant");

  auto const interactions = receiver.recorder().timedInteractions();
  auto const grants = receiver.recorder().timeAdvanceGrants();
  require(
      interactions.size() == 1U && grants.size() == 1U,
      "Regional NMR did not deliver exactly one interaction and one grant");
  auto const& interaction = interactions.front();
  require(
      interaction.interaction == receiverInteraction &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(receiverParameter) == 1U &&
          copyBytes(interaction.parameters.at(receiverParameter)) == parameterBytes &&
          interaction.tag == tagBytes &&
          interaction.producer == publisher.federateHandle() &&
          interaction.time == encodeTime(*timestamp) &&
          interaction.sentOrder == rti::TIMESTAMP &&
          interaction.receivedOrder == rti::TIMESTAMP &&
          interaction.retractionPresent,
      "Regional NMR did not preserve the selected timestamped interaction");
  require(
      grants.front().encoded == encodeTime(*timestamp) &&
          receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "grant"},
      "Regional NMR did not grant at the selected TSO timestamp after delivery");

  receiver.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      receiverInteraction,
      rti::RegionHandleSet{receiverRegion});
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.rtiAmbassador().deleteRegion(receiverRegion);
  publisher.rtiAmbassador().deleteRegion(publisherRegion);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioNmrRegionalTimestampedInteractionMessageSetContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrRegionalTimestampedInteractionMessageSet(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
