#include "tck_fqr_undefined_galt_nrg.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

void runOne(Options const& options, rti::CallbackModel model, bool queued) {
  Session requester(options, model, "fqr-undefined-galt-requester");
  Session producer(options, model, "fqr-undefined-galt-producer");
  auto const federation = federationName(
      options, queued ? "fqr-undefined-galt-queued" : "fqr-undefined-galt-empty");
  requester.connect();
  producer.connect();
  requester.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  requester.join(options.ownerFederateName, options.federateType, federation);
  producer.join(options.memberFederateName, options.federateType, federation);
  auto const nrgEnabled = requester.rtiAmbassador().getNonRegulatedGrantSwitch();
  require(producer.rtiAmbassador().getNonRegulatedGrantSwitch() == nrgEnabled,
          "federates disagreed on the adapter FOM non-regulated grant switch");

  auto const requesterTime = makeTimeContext(requester);
  auto const producerTime = makeTimeContext(producer);
  require(requesterTime.factory->getName() == producerTime.factory->getName(),
          "federates selected different logical-time implementations");
  requireTimeEquals(*requesterTime.initial, *producerTime.initial,
                    "undefined-GALT initial logical time");
  auto const messageTime = timeAfter(*requesterTime.factory, *requesterTime.initial,
                                     *requesterTime.epsilon, 5U);
  auto const requestTime = timeAfter(*requesterTime.factory, *requesterTime.initial,
                                     *requesterTime.epsilon, 10U);
  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(requester,
          [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "undefined-GALT time-constrained enablement");

  std::optional<rti::InteractionClassHandle> interaction;
  std::optional<rti::ParameterHandle> parameter;
  std::array<std::uint8_t, 1> const payload{'N'};
  std::array<std::uint8_t, 3> const tagBytes{'F', 'Q', 'R'};
  if (queued) {
    interaction = producer.rtiAmbassador().getInteractionClassHandle(
        options.interactionClassName);
    parameter = producer.rtiAmbassador().getParameterHandle(
        *interaction, options.parameterName);
    require(interaction->isValid() && parameter->isValid() &&
                requester.rtiAmbassador().getInteractionClassHandle(
                    options.interactionClassName) == *interaction,
            "adapter FOM lacks the ordinary interaction and parameter");
    producer.rtiAmbassador().publishInteractionClass(*interaction);
    producer.rtiAmbassador().changeInteractionOrderType(*interaction, rti::TIMESTAMP);
    requester.rtiAmbassador().subscribeInteractionClass(*interaction, true);
    producer.rtiAmbassador().enableTimeRegulation(*producerTime.epsilon);
    waitFor(producer,
            [&] { return producer.recorder().timeRegulationEnabled().size() == 1U; },
            options, "undefined-GALT producer regulation");
    rti::ParameterHandleValueMap values;
    values.emplace(*parameter, rti::VariableLengthData(payload.data(), payload.size()));
    rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    auto const retraction = producer.rtiAmbassador().sendInteraction(
        *interaction, values, tag, *messageTime);
    require(retraction.isValid(), "queued interaction lacks a retraction handle");
    producer.rtiAmbassador().disableTimeRegulation();
    producer.pump();
  }

  auto galt = requesterTime.factory->makeInitial();
  require(galt != nullptr && !requester.rtiAmbassador().queryGALT(*galt),
          "undefined-GALT FQR setup unexpectedly had a defined GALT");
  requester.recorder().clearCallbackOrder();
  requester.rtiAmbassador().flushQueueRequest(*requestTime);
  waitFor(requester,
          [&] {
            return requester.recorder().flushQueueGrants().size() == 1U &&
                (!queued || requester.recorder().timedInteractions().size() == 1U);
          }, options, "undefined-GALT Flush Queue Grant");
  auto const grants = requester.recorder().flushQueueGrants();
  auto const expectedOptimistic = queued ? messageTime.get() : requestTime.get();
  auto const expectedActual = nrgEnabled ? expectedOptimistic : requesterTime.initial.get();
  require(grants.size() == 1U &&
              grants.front().time == encodeTime(*expectedActual) &&
              grants.front().optimisticTime == encodeTime(*expectedOptimistic),
          "undefined-GALT FQG actual or optimistic time disagreed with the NRG switch");
  require(requester.recorder().callbackOrder() ==
              (queued ? std::vector<std::string>{"interaction", "flush-grant"}
                      : std::vector<std::string>{"flush-grant"}),
          "undefined-GALT FQR callback order differed from delivered message order");
  auto const delivered = requester.recorder().timedInteractions();
  require(delivered.size() == (queued ? 1U : 0U),
          "undefined-GALT FQR delivered the wrong number of interactions");
  if (queued) {
    require(delivered.front().interaction == *interaction &&
                delivered.front().parameters.size() == 1U &&
                delivered.front().parameters.count(*parameter) == 1U &&
                copyBytes(delivered.front().parameters.at(*parameter)) ==
                    std::vector<std::uint8_t>(payload.begin(), payload.end()) &&
                delivered.front().tag ==
                    std::vector<std::uint8_t>(tagBytes.begin(), tagBytes.end()) &&
                delivered.front().time == encodeTime(*messageTime) &&
                delivered.front().producer == producer.federateHandle() &&
                delivered.front().sentOrder == rti::TIMESTAMP &&
                delivered.front().receivedOrder == rti::TIMESTAMP,
            "undefined-GALT FQR changed the delivered interaction fields");
  }
  require(!requester.rtiAmbassador().queryGALT(*galt),
          "undefined-GALT FQR fabricated a GALT after its grant");
  auto logicalTime = requesterTime.factory->makeInitial();
  require(logicalTime != nullptr, "logical-time factory returned no query output");
  requester.rtiAmbassador().queryLogicalTime(*logicalTime);
  requireTimeEquals(*logicalTime, *expectedActual,
                    "undefined-GALT requester logical time after FQR");

  requester.rtiAmbassador().disableTimeConstrained();
  requester.resign(rti::NO_ACTION);
  producer.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  producer.disconnect();
  requester.disconnect();
}

void runMatrix(Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "undefined-GALT FQR requires adapter FOM and logical-time implementation");
  runOne(options, model, false);
  runOne(options, model, true);
}

}  // namespace

void scenarioFqrUndefinedGaltNrg(Options const& options, rti::CallbackModel model) {
  runMatrix(options, model);
}

void scenarioFqrUndefinedGaltNrgContract(Options const& options, rti::CallbackModel model) {
  runMatrix(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
