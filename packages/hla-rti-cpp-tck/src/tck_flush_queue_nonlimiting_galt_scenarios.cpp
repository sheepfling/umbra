#include "tck_flush_queue_nonlimiting_galt_scenarios.hpp"

#include "tck_main_scenarios.hpp"

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
      "Flush Queue non-limiting-GALT result");
  auto const grants = requester.recorder().flushQueueGrants();
  require(
      grants.size() == 1U &&
          grants.front().time == encodeTime(*requestTime) &&
          grants.front().optimisticTime == encodeTime(*requestTime),
      "Flush Queue actual and optimistic logical times differed while GALT exceeded the request");
  require(
      requester.recorder().callbackOrder() ==
          std::vector<std::string>{"flush-grant"},
      "Flush Queue non-limiting-GALT case delivered an unexpected callback");

  auto queriedLogicalTime = requesterTime.factory->makeInitial();
  require(
      queriedLogicalTime != nullptr,
      "Flush Queue non-limiting-GALT case could not allocate Query Logical Time output");
  requester.rtiAmbassador().queryLogicalTime(*queriedLogicalTime);
  requireTimeEquals(
      *queriedLogicalTime,
      *requestTime,
      "Flush Queue non-limiting-GALT logical time did not match its optimistic result");

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
