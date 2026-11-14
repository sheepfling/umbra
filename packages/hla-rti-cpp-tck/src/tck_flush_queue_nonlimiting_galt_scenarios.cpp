#include "tck_flush_queue_nonlimiting_galt_scenarios.hpp"

#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

void runFlushQueueNonlimitingGaltScenario(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Flush Queue non-limiting-GALT scenario requires an adapter-supplied logical-time implementation");

  Session regulator(options, model, "fqr-nonlimiting-galt-regulator");
  Session requester(options, model, "fqr-nonlimiting-galt-requester");
  auto const federation = federationName(
      options, "flush-queue-grant-optimistic-time-equal-when-galt-nonlimiting");
  regulator.connect();
  requester.connect();
  regulator.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  regulator.join(options.ownerFederateName, options.federateType, federation);
  requester.join(
      options.memberFederateName + L"-fqr-nonlimiting-galt",
      options.federateType,
      federation);

  auto const regulatorTime = makeTimeContext(regulator);
  auto const requesterTime = makeTimeContext(requester);
  require(
      regulatorTime.factory->getName() == requesterTime.factory->getName(),
      "Flush Queue non-limiting-GALT federates selected different logical-time factories");
  requireTimeEquals(
      *regulatorTime.initial,
      *requesterTime.initial,
      "Flush Queue non-limiting-GALT initial time");

  auto const regulatorInteraction = regulator.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const requesterInteraction = requester.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const regulatorParameter = regulator.rtiAmbassador().getParameterHandle(
      regulatorInteraction,
      options.parameterName);
  auto const requesterParameter = requester.rtiAmbassador().getParameterHandle(
      requesterInteraction,
      options.parameterName);
  require(
      regulatorInteraction.isValid() && requesterInteraction.isValid() &&
          regulatorParameter.isValid() && requesterParameter.isValid(),
      "Flush Queue non-limiting-GALT interaction lookup returned an invalid handle");
  regulator.rtiAmbassador().publishInteractionClass(regulatorInteraction);
  regulator.rtiAmbassador().changeInteractionOrderType(
      regulatorInteraction,
      rti::TIMESTAMP);
  requester.rtiAmbassador().subscribeInteractionClass(requesterInteraction, true);

  std::vector<std::uint8_t> const payload{0x46U, 0x51U, 0x52U};
  std::vector<std::uint8_t> const tagBytes{0x46U, 0x51U, 0x52U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      regulatorParameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());

  regulator.rtiAmbassador().enableTimeRegulation(*regulatorTime.epsilon);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Flush Queue non-limiting-GALT regulation callback");
  auto const regulatorTarget = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      12U);
  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorTarget);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Flush Queue non-limiting-GALT regulator grant");
  require(
      regulator.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*regulatorTarget),
      "Flush Queue non-limiting-GALT regulator received the wrong grant time");
  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "Flush Queue non-limiting-GALT constrained callback");
  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  auto observedGalt = requesterTime.factory->makeInitial();
  require(
      observedGalt != nullptr,
      "Flush Queue non-limiting-GALT case could not allocate Query GALT output");
  require(
      requester.rtiAmbassador().queryGALT(*observedGalt) &&
      *requestTime < *observedGalt,
      "Flush Queue setup did not establish a defined GALT strictly above its request");

  requester.recorder().clearCallbackOrder();
  requester.rtiAmbassador().flushQueueRequest(*requestTime);
  waitFor(
      requester,
      [&] { return requester.recorder().flushQueueGrants().size() == 1U; },
      options,
      "Flush Queue non-limiting-GALT request-only result");
  auto const grants = requester.recorder().flushQueueGrants();
  require(
      grants.size() == 1U &&
          grants.front().time == encodeTime(*requestTime) &&
          grants.front().optimisticTime == encodeTime(*requestTime) &&
          requester.recorder().timedInteractions().empty(),
      "Flush Queue actual and optimistic logical times differed from the request while GALT exceeded it and no TSO message was queued");
  require(
      requester.recorder().callbackOrder() ==
          std::vector<std::string>{"flush-grant"},
      "Flush Queue non-limiting-GALT request-only case delivered an unexpected callback");

  auto queriedLogicalTime = requesterTime.factory->makeInitial();
  require(
      queriedLogicalTime != nullptr,
      "Flush Queue non-limiting-GALT case could not allocate Query Logical Time output");
  requester.rtiAmbassador().queryLogicalTime(*queriedLogicalTime);
  requireTimeEquals(
      *queriedLogicalTime,
      *requestTime,
      "Flush Queue non-limiting-GALT logical time did not match its request-only grant");

  auto const messageTime = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      15U);
  auto const retraction = regulator.rtiAmbassador().sendInteraction(
      regulatorInteraction,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "Flush Queue TSO-minimum send returned an invalid retraction handle");
  require(
      requester.recorder().timedInteractions().empty(),
      "Flush Queue TSO-minimum interaction was delivered before the second FQR");
  auto const secondRegulatorTarget = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      30U);
  regulator.rtiAmbassador().timeAdvanceRequest(*secondRegulatorTarget);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "Flush Queue TSO-minimum regulator grant");
  require(
      regulator.recorder().timeAdvanceGrants().back().encoded ==
          encodeTime(*secondRegulatorTarget),
      "Flush Queue TSO-minimum regulator received the wrong second grant time");

  auto const secondRequestTime = timeAfter(
      *requesterTime.factory,
      *requestTime,
      *requesterTime.epsilon,
      10U);
  auto secondGalt = requesterTime.factory->makeInitial();
  auto expectedGrantTime = requesterTime.factory->makeInitial();
  require(
      secondGalt != nullptr && expectedGrantTime != nullptr,
      "Flush Queue TSO-minimum case could not allocate logical-time outputs");
  require(
      *messageTime < *secondRequestTime,
      "Flush Queue TSO-minimum setup did not place the message before its request");
  require(
      requester.rtiAmbassador().queryGALT(*secondGalt),
      "Flush Queue TSO-minimum setup did not establish a defined GALT");
  *expectedGrantTime = *secondRequestTime;
  if (*secondGalt < *expectedGrantTime) {
    *expectedGrantTime = *secondGalt;
  }
  if (*messageTime < *expectedGrantTime) {
    *expectedGrantTime = *messageTime;
  }

  requester.recorder().clearCallbackOrder();
  requester.rtiAmbassador().flushQueueRequest(*secondRequestTime);
  waitFor(
      requester,
      [&] {
        return requester.recorder().timedInteractions().size() == 1U &&
            requester.recorder().flushQueueGrants().size() == 2U;
      },
      options,
      "Flush Queue TSO-minimum result");
  auto const secondGrants = requester.recorder().flushQueueGrants();
  auto const interactions = requester.recorder().timedInteractions();
  auto const expectedGrantEncoding = encodeTime(*expectedGrantTime);
  auto const deliveredTime = encodeTime(*messageTime);
  require(
      secondGrants.size() == 2U &&
          secondGrants.back().time == expectedGrantEncoding &&
          secondGrants.back().optimisticTime == expectedGrantEncoding,
      "Flush Queue actual and optimistic logical times did not equal the minimum of request, GALT, and delivered TSO timestamps");
  require(
      interactions.size() == 1U &&
          interactions.front().interaction == requesterInteraction &&
          interactions.front().parameters.size() == 1U &&
          interactions.front().parameters.count(requesterParameter) == 1U &&
          copyBytes(interactions.front().parameters.at(requesterParameter)) == payload &&
          interactions.front().tag == tagBytes &&
          interactions.front().time == deliveredTime &&
          interactions.front().producer == regulator.federateHandle() &&
          interactions.front().sentOrder == rti::TIMESTAMP &&
          interactions.front().receivedOrder == rti::TIMESTAMP &&
          interactions.front().retractionPresent &&
          interactions.front().retraction == copyBytes(retraction.encode()),
      "Flush Queue TSO-minimum callback changed the interaction data or timestamp");
  require(
      requester.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "flush-grant"},
      "Flush Queue TSO-minimum grant preceded the delivered interaction callback");
  requester.rtiAmbassador().queryLogicalTime(*queriedLogicalTime);
  requireTimeEquals(
      *queriedLogicalTime,
      *expectedGrantTime,
      "Flush Queue TSO-minimum logical time did not match its actual and optimistic grant");

  requester.rtiAmbassador().disableTimeConstrained();
  regulator.rtiAmbassador().disableTimeRegulation();
  requester.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  regulator.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  regulator.disconnect();
}

}  // namespace

void scenarioFlushQueueNonlimitingGalt(
    Options const& options,
    rti::CallbackModel model) {
  runFlushQueueNonlimitingGaltScenario(options, model);
}

void scenarioFlushQueueNonlimitingGaltContract(
    Options const& options,
    rti::CallbackModel model) {
  runFlushQueueNonlimitingGaltScenario(options, model);
}

void scenarioFlushQueueNonlimitingGaltPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFlushQueueNonlimitingGalt(options, model);
}

void scenarioFlushQueueNonlimitingGaltContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFlushQueueNonlimitingGaltContract(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
