#include "tck_main_scenarios.hpp"
#include "tck_nmr_process_coordination.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

void runUndefinedGaltNmr(
    Options const& options,
    rti::CallbackModel model,
    bool expectedNrgEnabled) {
  auto const switchName = expectedNrgEnabled ? "enabled" : "disabled";
  Session regulator(options, model, "nmr-nrg-regulator");
  Session requester(options, model, "nmr-nrg-requester");
  auto const federation = federationName(
      options,
      std::string("nmr-undefined-galt-nrg-") + switchName);
  connectAndJoin(regulator, requester, options, federation, options.fom);

  auto const nrgEnabled = regulator.rtiAmbassador().getNonRegulatedGrantSwitch();
  require(nrgEnabled == expectedNrgEnabled,
          "NMR adapter FOM did not match its expected NRG switch");
  require(requester.rtiAmbassador().getNonRegulatedGrantSwitch() == nrgEnabled,
          "NMR federates disagreed on the NRG switch");

  auto const regulatorInteraction = regulator.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const regulatorParameter = regulator.rtiAmbassador().getParameterHandle(
      regulatorInteraction, options.parameterName);
  auto const requesterInteraction = requester.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const requesterParameter = requester.rtiAmbassador().getParameterHandle(
      requesterInteraction, options.parameterName);
  require(regulatorInteraction.isValid() && regulatorParameter.isValid() &&
              requesterInteraction.isValid() && requesterParameter.isValid() &&
              regulatorInteraction == requesterInteraction &&
              regulatorParameter == requesterParameter,
          "NMR undefined-GALT FOM did not resolve consistent interaction handles");
  regulator.rtiAmbassador().publishInteractionClass(regulatorInteraction);
  regulator.rtiAmbassador().changeInteractionOrderType(
      regulatorInteraction, rti::TIMESTAMP);
  requester.rtiAmbassador().subscribeInteractionClass(requesterInteraction, true);

  auto regulatorTime = makeTimeContext(regulator);
  auto requesterTime = makeTimeContext(requester);
  require(regulatorTime.factory->getName() == requesterTime.factory->getName(),
          "NMR undefined-GALT members selected different logical-time factories");
  requireTimeEquals(*regulatorTime.initial, *requesterTime.initial,
                    "NMR undefined-GALT initial logical time");
  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(requester,
          [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "NMR undefined-GALT requester Time Constrained callback");

  auto const lookahead = regulatorTime.factory->makeZero();
  require(lookahead != nullptr, "NMR undefined-GALT could not allocate lookahead");
  lookahead->setToDifference(
      *timeAfter(*regulatorTime.factory, *regulatorTime.initial,
                 *regulatorTime.epsilon, 1U),
      *regulatorTime.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(regulator,
          [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
          options, "NMR undefined-GALT regulator Time Regulation callback");

  auto const messageTime = timeAfter(
      *regulatorTime.factory, *regulatorTime.initial,
      *regulatorTime.epsilon, 3U);
  auto const regulatorTarget = timeAfter(
      *regulatorTime.factory, *regulatorTime.initial,
      *regulatorTime.epsilon, 4U);
  auto const requestTime = timeAfter(
      *requesterTime.factory, *requesterTime.initial,
      *requesterTime.epsilon, 10U);
  std::vector<std::uint8_t> const payloadBytes{0x4EU, 0x4DU, 0x52U};
  std::vector<std::uint8_t> const tagBytes{0x4EU, 0x52U, 0x47U};
  rti::ParameterHandleValueMap values;
  values.emplace(regulatorParameter,
                 rti::VariableLengthData(payloadBytes.data(), payloadBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = regulator.rtiAmbassador().sendInteraction(
      regulatorInteraction, values, tag, *messageTime);
  require(retraction.isValid(),
          "NMR undefined-GALT send returned an invalid retraction handle");
  regulator.rtiAmbassador().disableTimeRegulation();
  regulator.pump();

  auto galt = requesterTime.factory->makeInitial();
  require(galt != nullptr && !requester.rtiAmbassador().queryGALT(*galt),
          "NMR undefined-GALT setup unexpectedly had a defined GALT");
  require(requester.recorder().timedInteractions().empty() &&
              requester.recorder().timeAdvanceGrants().empty(),
          "NMR undefined-GALT TSO was delivered before the request");
  requester.rtiAmbassador().nextMessageRequest(*requestTime);

  if (!expectedNrgEnabled) {
    auto const observationMilliseconds =
        std::max(1, std::min(options.timeoutMilliseconds, 250));
    auto const quietDeadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(observationMilliseconds);
    while (std::chrono::steady_clock::now() < quietDeadline) {
      requester.pump();
      regulator.pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(requester.recorder().timedInteractions().empty() &&
                requester.recorder().timeAdvanceGrants().empty(),
            "NMR with undefined GALT and NRG disabled advanced beyond current time");
    auto requesterCurrentTime = requesterTime.factory->makeInitial();
    require(requesterCurrentTime != nullptr,
            "NMR undefined-GALT could not allocate a logical-time query value");
    requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
    requireTimeEquals(*requesterCurrentTime, *requesterTime.initial,
                      "NMR with NRG disabled before regulator resumes");

    regulator.rtiAmbassador().enableTimeRegulation(*lookahead);
    waitFor(regulator,
            [&] { return regulator.recorder().timeRegulationEnabled().size() == 2U; },
            options, "NMR resumed regulator Time Regulation callback");
    regulator.rtiAmbassador().timeAdvanceRequest(*regulatorTarget);
    waitFor(regulator,
            [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
            options, "NMR resumed regulator advance");
    require(regulator.recorder().timeAdvanceGrants().front().encoded ==
                encodeTime(*regulatorTarget),
            "NMR resumed regulator did not reach its requested time");
  }

  waitFor(requester,
          [&] {
            return requester.recorder().timedInteractions().size() == 1U &&
                requester.recorder().timeAdvanceGrants().size() == 1U;
          },
          options, "NMR undefined-GALT delivery and grant");
  auto const received = requester.recorder().timedInteractions().front();
  require(received.interaction == requesterInteraction &&
              received.parameters.size() == 1U &&
              received.parameters.count(requesterParameter) == 1U &&
              copyBytes(received.parameters.at(requesterParameter)) == payloadBytes &&
              received.tag == tagBytes && received.producer == regulator.federateHandle(),
          "NMR undefined-GALT delivered the wrong interaction payload or producer");
  require(received.time == encodeTime(*messageTime) &&
              received.sentOrder == rti::TIMESTAMP &&
              received.receivedOrder == rti::TIMESTAMP,
          "NMR undefined-GALT changed queued TSO time or order metadata");
  require(requester.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "grant"},
          "NMR undefined-GALT granted before delivering its queued TSO");
  auto const grants = requester.recorder().timeAdvanceGrants();
  require(grants.size() == 1U && grants.front().encoded == encodeTime(*messageTime),
          "NMR undefined-GALT did not grant at the queued TSO timestamp");
  require(regulator.recorder().timeAdvanceGrants().size() ==
              (expectedNrgEnabled ? 0U : 1U),
          "NMR undefined-GALT used the wrong number of regulator advances");

  auto requesterCurrentTime = requesterTime.factory->makeInitial();
  require(requesterCurrentTime != nullptr,
          "NMR undefined-GALT could not allocate the final logical-time query value");
  requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
  requireTimeEquals(*requesterCurrentTime, *messageTime,
                    "NMR undefined-GALT requester time at LITS");
  requester.rtiAmbassador().disableTimeConstrained();
  requester.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
}

void runUndefinedGaltNmrMatrix(
    Options const& options,
    rti::CallbackModel model) {
  auto const disabledFom = options.scenarioFoms.find("nmr-undefined-galt-nrg-disabled");
  auto const enabledFom = options.scenarioFoms.find("nmr-undefined-galt-nrg-enabled");
  require(disabledFom != options.scenarioFoms.end() && !disabledFom->second.empty() &&
              enabledFom != options.scenarioFoms.end() && !enabledFom->second.empty(),
          "NMR undefined-GALT matrix requires NRG-disabled and NRG-enabled adapter FOMs");
  auto disabledOptions = options;
  disabledOptions.fom = disabledFom->second;
  runUndefinedGaltNmr(disabledOptions, model, false);
  auto enabledOptions = options;
  enabledOptions.fom = enabledFom->second;
  runUndefinedGaltNmr(enabledOptions, model, true);
}

void runNoTsoUndefinedGaltNmr(
    Options const& options,
    rti::CallbackModel model,
    bool expectedNrgEnabled) {
  auto const switchName = expectedNrgEnabled ? "enabled" : "disabled";
  Session regulator(options, model, "nmr-no-tso-nrg-regulator");
  Session secondRegulator(options, model, "nmr-no-tso-nrg-second-regulator");
  Session requester(options, model, "nmr-no-tso-nrg-requester");
  auto const federation = federationName(
      options, std::string("nmr-no-tso-undefined-galt-nrg-") + switchName);
  regulator.connect();
  secondRegulator.connect();
  requester.connect();
  regulator.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  regulator.join(options.ownerFederateName, options.federateType, federation);
  secondRegulator.join(
      options.ownerFederateName + L"-second-regulator",
      options.federateType,
      federation);
  requester.join(options.memberFederateName, options.federateType, federation);

  auto const nrgEnabled = regulator.rtiAmbassador().getNonRegulatedGrantSwitch();
  require(nrgEnabled == expectedNrgEnabled,
          "NMR no-TSO adapter FOM did not match its expected NRG switch");
  require(requester.rtiAmbassador().getNonRegulatedGrantSwitch() == nrgEnabled,
          "NMR no-TSO federates disagreed on the NRG switch");
  require(secondRegulator.rtiAmbassador().getNonRegulatedGrantSwitch() == nrgEnabled,
          "NMR no-TSO regulators disagreed on the NRG switch");

  auto regulatorTime = makeTimeContext(regulator);
  auto secondRegulatorTime = makeTimeContext(secondRegulator);
  auto requesterTime = makeTimeContext(requester);
  require(regulatorTime.factory->getName() == requesterTime.factory->getName() &&
              secondRegulatorTime.factory->getName() ==
                  requesterTime.factory->getName(),
          "NMR no-TSO members selected different logical-time factories");
  requireTimeEquals(*regulatorTime.initial, *requesterTime.initial,
                    "NMR no-TSO initial logical time");
  requireTimeEquals(*secondRegulatorTime.initial, *requesterTime.initial,
                    "NMR no-TSO second regulator initial logical time");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(requester,
          [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "NMR no-TSO requester Time Constrained callback");

  auto const initialGalt = timeAfter(
      *regulatorTime.factory, *regulatorTime.initial,
      *regulatorTime.epsilon, 1U);
  auto const primaryRegulatorInitialGalt = timeAfter(
      *regulatorTime.factory, *regulatorTime.initial,
      *regulatorTime.epsilon, 2U);
  auto primaryRegulatorLookahead = regulatorTime.factory->makeZero();
  auto secondRegulatorLookahead = secondRegulatorTime.factory->makeZero();
  require(primaryRegulatorLookahead != nullptr &&
              secondRegulatorLookahead != nullptr,
          "NMR no-TSO regulators could not allocate lookahead intervals");
  primaryRegulatorLookahead->setToDifference(
      *primaryRegulatorInitialGalt, *regulatorTime.initial);
  secondRegulatorLookahead->setToDifference(
      *initialGalt, *secondRegulatorTime.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*primaryRegulatorLookahead);
  waitFor(regulator,
          [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
          options, "NMR no-TSO primary regulator Time Regulation callback");
  secondRegulator.rtiAmbassador().enableTimeRegulation(*secondRegulatorLookahead);
  waitFor(secondRegulator,
          [&] {
            return secondRegulator.recorder().timeRegulationEnabled().size() ==
                1U;
          },
          options, "NMR no-TSO second regulator Time Regulation callback");

  auto galt = requesterTime.factory->makeInitial();
  require(galt != nullptr && requester.rtiAmbassador().queryGALT(*galt),
          "NMR no-TSO setup did not establish a defined GALT");
  requireTimeEquals(*galt, *initialGalt, "NMR no-TSO initial GALT");

  auto const requestTime = timeAfter(
      *requesterTime.factory, *requesterTime.initial,
      *requesterTime.epsilon, 10U);
  regulator.rtiAmbassador().disableTimeRegulation();
  secondRegulator.rtiAmbassador().disableTimeRegulation();
  regulator.pump();
  secondRegulator.pump();
  require(!requester.rtiAmbassador().queryGALT(*galt),
          "NMR no-TSO setup unexpectedly retained a defined GALT");
  requester.rtiAmbassador().nextMessageRequest(*requestTime);

  if (!expectedNrgEnabled) {
    auto const observationMilliseconds =
        std::max(1, std::min(options.timeoutMilliseconds, 250));
    auto const quietDeadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(observationMilliseconds);
    while (std::chrono::steady_clock::now() < quietDeadline) {
      requester.pump();
      regulator.pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(requester.recorder().timeAdvanceGrants().empty() &&
                requester.recorder().timedInteractions().empty(),
            "NMR with undefined GALT and NRG disabled advanced without a regulator");
    auto requesterCurrentTime = requesterTime.factory->makeInitial();
    require(requesterCurrentTime != nullptr,
            "NMR no-TSO could not allocate its current logical-time query value");
    requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
    requireTimeEquals(*requesterCurrentTime, *requesterTime.initial,
                      "NMR no-TSO NRG-disabled time before regulator resumes");

    regulator.rtiAmbassador().enableTimeRegulation(*primaryRegulatorLookahead);
    waitFor(regulator,
            [&] { return regulator.recorder().timeRegulationEnabled().size() == 2U; },
            options,
            "NMR no-TSO resumed primary regulator Time Regulation callback");
    secondRegulator.rtiAmbassador().enableTimeRegulation(
        *secondRegulatorLookahead);
    waitFor(secondRegulator,
            [&] {
              return secondRegulator.recorder().timeRegulationEnabled().size() ==
                  2U;
            },
            options,
            "NMR no-TSO resumed second regulator Time Regulation callback");
    auto const regulatorTarget = timeAfter(
        *regulatorTime.factory, *regulatorTime.initial,
        *regulatorTime.epsilon, 11U);
    regulator.rtiAmbassador().timeAdvanceRequest(*regulatorTarget);
    waitFor(regulator,
            [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
            options, "NMR no-TSO primary regulator advance");
    require(regulator.recorder().timeAdvanceGrants().front().encoded ==
                encodeTime(*regulatorTarget),
            "NMR no-TSO primary regulator missed its requested time");

    auto const secondRegulatorQuietDeadline =
        std::chrono::steady_clock::now() +
        std::chrono::milliseconds(observationMilliseconds);
    while (std::chrono::steady_clock::now() < secondRegulatorQuietDeadline) {
      requester.pump();
      secondRegulator.pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(requester.recorder().timeAdvanceGrants().empty() &&
                requester.recorder().timedInteractions().empty(),
            "NMR granted while the second regulator still limited GALT");
    auto requesterCurrentTimeAfterFirstRegulator =
        requesterTime.factory->makeInitial();
    require(requesterCurrentTimeAfterFirstRegulator != nullptr,
            "NMR no-TSO could not allocate its first-regulator time query");
    requester.rtiAmbassador().queryLogicalTime(
        *requesterCurrentTimeAfterFirstRegulator);
    requireTimeEquals(*requesterCurrentTimeAfterFirstRegulator,
                      *requesterTime.initial,
                      "NMR no-TSO time after only one regulator advanced");
    auto const galtWhileSecondRegulatorIsBehind =
        requesterTime.factory->makeInitial();
    require(galtWhileSecondRegulatorIsBehind != nullptr &&
                requester.rtiAmbassador().queryGALT(
                    *galtWhileSecondRegulatorIsBehind),
            "NMR no-TSO lost GALT while the second regulator remained active");
    requireTimeEquals(
        *galtWhileSecondRegulatorIsBehind,
        *initialGalt,
        "NMR no-TSO GALT after only the primary regulator advanced");

    auto const secondRegulatorTarget = timeAfter(
        *secondRegulatorTime.factory,
        *secondRegulatorTime.initial,
        *secondRegulatorTime.epsilon,
        11U);
    secondRegulator.rtiAmbassador().timeAdvanceRequest(*secondRegulatorTarget);
    waitFor(secondRegulator,
            [&] {
              return secondRegulator.recorder().timeAdvanceGrants().size() ==
                  1U;
            },
            options, "NMR no-TSO second regulator advance");
    require(secondRegulator.recorder().timeAdvanceGrants().front().encoded ==
                encodeTime(*secondRegulatorTarget),
            "NMR no-TSO second regulator missed its requested time");
    auto const galtAfterBothRegulatorsAdvance =
        requesterTime.factory->makeInitial();
    require(galtAfterBothRegulatorsAdvance != nullptr &&
                requester.rtiAmbassador().queryGALT(
                    *galtAfterBothRegulatorsAdvance),
            "NMR no-TSO lost GALT after both regulators advanced");
    require(*galtAfterBothRegulatorsAdvance > *requestTime,
            "NMR no-TSO GALT did not clear the request after both regulators advanced");
  }

  waitFor(requester,
          [&] { return requester.recorder().timeAdvanceGrants().size() == 1U; },
          options, "NMR no-TSO grant at the requested time");
  require(requester.recorder().timedInteractions().empty(),
          "NMR no-TSO grant unexpectedly delivered a TSO");
  auto const grants = requester.recorder().timeAdvanceGrants();
  require(grants.size() == 1U && grants.front().encoded == encodeTime(*requestTime),
          "NMR no-TSO did not grant exactly at its request limit");
  require(requester.recorder().callbackOrder() == std::vector<std::string>{"grant"},
          "NMR no-TSO produced an unexpected callback order");
  require(regulator.recorder().timeAdvanceGrants().size() ==
              (expectedNrgEnabled ? 0U : 1U),
          "NMR no-TSO primary regulator advanced unexpectedly");
  require(secondRegulator.recorder().timeAdvanceGrants().size() ==
              (expectedNrgEnabled ? 0U : 1U),
          "NMR no-TSO second regulator advanced unexpectedly");

  auto requesterCurrentTime = requesterTime.factory->makeInitial();
  require(requesterCurrentTime != nullptr,
          "NMR no-TSO could not allocate its final logical-time query value");
  requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
  requireTimeEquals(*requesterCurrentTime, *requestTime,
                    "NMR no-TSO final requester time");
  if (expectedNrgEnabled) {
    require(!requester.rtiAmbassador().queryGALT(*galt),
            "NMR no-TSO NRG-enabled grant unexpectedly defined GALT");
  }

  requester.rtiAmbassador().disableTimeConstrained();
  requester.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  secondRegulator.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
}

void runNoTsoUndefinedGaltNmrMatrix(
    Options const& options,
    rti::CallbackModel model) {
  auto const disabledFom = options.scenarioFoms.find("nmr-undefined-galt-nrg-disabled");
  auto const enabledFom = options.scenarioFoms.find("nmr-undefined-galt-nrg-enabled");
  require(disabledFom != options.scenarioFoms.end() && !disabledFom->second.empty() &&
              enabledFom != options.scenarioFoms.end() && !enabledFom->second.empty(),
          "NMR no-TSO matrix requires NRG-disabled and NRG-enabled adapter FOMs");
  auto disabledOptions = options;
  disabledOptions.fom = disabledFom->second;
  runNoTsoUndefinedGaltNmr(disabledOptions, model, false);
  auto enabledOptions = options;
  enabledOptions.fom = enabledFom->second;
  runNoTsoUndefinedGaltNmr(enabledOptions, model, true);
}

}  // namespace

void scenarioNmrUndefinedGaltNrgMatrix(
    Options const& options,
    rti::CallbackModel model) {
  runUndefinedGaltNmrMatrix(options, model);
}

void scenarioNmrNoTsoUndefinedGaltNrg(
    Options const& options,
    rti::CallbackModel model) {
  runNoTsoUndefinedGaltNmrMatrix(options, model);
}

void scenarioNmrNoTsoUndefinedGaltNrgContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrNoTsoUndefinedGaltNrg(options, model);
}

namespace {

void requireNmraFutureArrivalQuiet(
    Session& requester,
    Options const& options,
    std::size_t expectedInteractionCount,
    std::string const& context) {
  auto const quietMilliseconds =
      std::max(1, std::min(options.timeoutMilliseconds, 250));
  auto const quietDeadline = Clock::now() +
      std::chrono::milliseconds(quietMilliseconds);
  while (Clock::now() < quietDeadline) {
    requester.pump();
    require(
        requester.recorder().timeAdvanceGrants().empty() &&
            requester.recorder().timedInteractions().size() ==
                expectedInteractionCount,
        context);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void requireNmraFutureArrivalDelivery(
    Session& requester,
    rti::FederateHandle const& expectedPeer,
    rti::InteractionClassHandle const& interaction,
    rti::ParameterHandle const& parameter,
    std::size_t interactionIndex,
    std::vector<std::uint8_t> const& payload,
    std::vector<std::uint8_t> const& tag,
    rti::LogicalTime const& messageTime,
    std::string const& context) {
  auto const& interactions = requester.recorder().timedInteractions();
  require(
      interactions.size() == interactionIndex + 1U,
      context + ": requester received duplicate or incomplete callbacks");
  auto const& received = interactions.back();
  require(
      received.interaction == interaction &&
          received.parameters.size() == 1U &&
          received.parameters.count(parameter) == 1U &&
          copyBytes(received.parameters.at(parameter)) == payload &&
          received.tag == tag && received.producer == expectedPeer,
      context + ": callback changed interaction payload or producer");
  require(
      received.time == encodeTime(messageTime) &&
          received.sentOrder == rti::TIMESTAMP &&
          received.receivedOrder == rti::TIMESTAMP &&
          received.retractionPresent,
      context + ": callback changed TSO timestamp or metadata");
  require(
      requester.recorder().timeAdvanceGrants().size() == 1U &&
          requester.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(messageTime) &&
          requester.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "grant"},
      context + ": delivery did not precede one grant at the message time");
}

void runNmraFutureArrivalRequesterProcess(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& directory) {
  Session requester(options, model, "owner");
  requester.connect();
  auto const federation = federationName(
      options, "nmra-future-tso-arrival-pending-request");
  requester.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  writeNmrProcessMarker(directory, "federation-created.marker");
  requester.join(options.ownerFederateName, options.federateType, federation);

  auto const interaction = requester.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = requester.rtiAmbassador().getParameterHandle(
      interaction, options.parameterName);
  require(
      interaction.isValid() && parameter.isValid(),
      "process-separated NMRA future-arrival requester FOM lookup failed");
  requester.rtiAmbassador().subscribeInteractionClass(interaction, true);

  auto requesterTime = makeTimeContext(requester);
  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "process-separated NMRA future-arrival Time Constrained callback");
  waitForNmrProcessMarker(directory, "publisher-ready.marker", options);
  auto const expectedPeer = requester.rtiAmbassador().getFederateHandle(
      options.memberFederateName);

  auto const requestBoundary = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  auto const firstMessageTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      5U);
  std::vector<std::uint8_t> const firstPayload{0x4EU, 0x4DU, 0x52U, 0x41U};
  std::vector<std::uint8_t> const firstTag{
      0x4EU, 0x4DU, 0x52U, 0x41U, 0x2DU, 0x46U};

  requester.recorder().clearTimeCallbacks();
  requester.recorder().clearCallbackOrder();
  requester.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  writeNmrProcessMarker(directory, "requester-first-pending.marker");
  requireNmraFutureArrivalQuiet(
      requester,
      options,
      0U,
      "process-separated NMRA completed before a qualifying TSO arrived");
  writeNmrProcessMarker(directory, "requester-first-pending-observed.marker");
  waitForNmrProcessMarker(directory, "publisher-first-tso-sent.marker", options);
  requireNmraFutureArrivalQuiet(
      requester,
      options,
      0U,
      "process-separated NMRA crossed the regulator boundary before advance");
  writeNmrProcessMarker(directory, "requester-first-tso-quiet.marker");
  waitForNmrProcessMarker(directory, "publisher-first-advance.marker", options);
  waitFor(
      requester,
      [&] {
        return requester.recorder().timeAdvanceGrants().size() >= 1U &&
            requester.recorder().timedInteractions().size() >= 1U;
      },
      options,
      "process-separated NMRA future-arrival delivery and grant");
  requireNmraFutureArrivalDelivery(
      requester,
      expectedPeer,
      interaction,
      parameter,
      0U,
      firstPayload,
      firstTag,
      *firstMessageTime,
      "process-separated NMRA first future arrival");
  auto requesterCurrentTime = requesterTime.factory->makeInitial();
  require(
      requesterCurrentTime != nullptr,
      "process-separated NMRA requester could not query first grant time");
  requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
  requireTimeEquals(
      *requesterCurrentTime,
      *firstMessageTime,
      "process-separated NMRA requester first grant time");

  requester.recorder().clearTimeCallbacks();
  requester.recorder().clearCallbackOrder();
  requester.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  writeNmrProcessMarker(directory, "requester-boundary-pending.marker");
  requireNmraFutureArrivalQuiet(
      requester,
      options,
      1U,
      "process-separated NMRA boundary request completed before its TSO arrived");
  writeNmrProcessMarker(directory, "requester-boundary-pending-observed.marker");
  waitForNmrProcessMarker(directory, "publisher-boundary-tso-sent.marker", options);
  requireNmraFutureArrivalQuiet(
      requester,
      options,
      1U,
      "process-separated NMRA delivered the boundary TSO before regulator advance");
  writeNmrProcessMarker(directory, "requester-boundary-tso-quiet.marker");
  waitForNmrProcessMarker(directory, "publisher-boundary-advance.marker", options);
  waitFor(
      requester,
      [&] {
        return requester.recorder().timeAdvanceGrants().size() >= 1U &&
            requester.recorder().timedInteractions().size() >= 2U;
      },
      options,
      "process-separated NMRA inclusive-boundary delivery and grant");
  auto const boundaryMessageTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  std::vector<std::uint8_t> const boundaryPayload{0x42U, 0x4EU, 0x44U};
  std::vector<std::uint8_t> const boundaryTag{
      0x42U, 0x4EU, 0x44U, 0x2DU, 0x46U};
  requireNmraFutureArrivalDelivery(
      requester,
      expectedPeer,
      interaction,
      parameter,
      1U,
      boundaryPayload,
      boundaryTag,
      *boundaryMessageTime,
      "process-separated NMRA inclusive boundary arrival");
  requesterCurrentTime = requesterTime.factory->makeInitial();
  require(
      requesterCurrentTime != nullptr,
      "process-separated NMRA requester could not query boundary grant time");
  requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
  requireTimeEquals(
      *requesterCurrentTime,
      *boundaryMessageTime,
      "process-separated NMRA requester boundary grant time");
  writeNmrProcessMarker(directory, "requester-complete.marker");

  waitForNmrProcessMarker(directory, "publisher-resigned.marker", options);
  requester.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
}

void runNmraFutureArrivalPublisherProcess(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& directory) {
  waitForNmrProcessMarker(directory, "federation-created.marker", options);
  Session publisher(options, model, "member");
  publisher.connect();
  auto const federation = federationName(
      options, "nmra-future-tso-arrival-pending-request");
  publisher.join(options.memberFederateName, options.federateType, federation);

  auto const interaction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = publisher.rtiAmbassador().getParameterHandle(
      interaction, options.parameterName);
  require(
      interaction.isValid() && parameter.isValid(),
      "process-separated NMRA future-arrival publisher FOM lookup failed");
  publisher.rtiAmbassador().publishInteractionClass(interaction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      interaction, rti::TIMESTAMP);

  auto publisherTime = makeTimeContext(publisher);
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "process-separated NMRA future-arrival Time Regulation callback");
  publisher.recorder().clearTimeCallbacks();
  writeNmrProcessMarker(directory, "publisher-ready.marker");

  auto sendTimestampedInteraction = [&](rti::LogicalTime const& time,
                                        std::vector<std::uint8_t> const& payload,
                                        std::vector<std::uint8_t> const& tag,
                                        std::string const& context) {
    rti::ParameterHandleValueMap values;
    values.emplace(
        parameter,
        rti::VariableLengthData(payload.data(), payload.size()));
    rti::VariableLengthData const encodedTag(tag.data(), tag.size());
    auto const retraction = publisher.rtiAmbassador().sendInteraction(
        interaction, values, encodedTag, time);
    require(retraction.isValid(), context + ": invalid retraction handle");
  };

  waitForNmrProcessMarker(
      directory, "requester-first-pending-observed.marker", options);
  auto const firstMessageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const firstTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  std::vector<std::uint8_t> const firstPayload{0x4EU, 0x4DU, 0x52U, 0x41U};
  std::vector<std::uint8_t> const firstTag{
      0x4EU, 0x4DU, 0x52U, 0x41U, 0x2DU, 0x46U};
  sendTimestampedInteraction(
      *firstMessageTime, firstPayload, firstTag, "first NMRA future TSO");
  writeNmrProcessMarker(directory, "publisher-first-tso-sent.marker");
  waitForNmrProcessMarker(directory, "requester-first-tso-quiet.marker", options);
  publisher.rtiAmbassador().timeAdvanceRequest(*firstTarget);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "process-separated NMRA publisher first grant");
  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U &&
          publisher.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*firstTarget),
      "process-separated NMRA publisher missed its first target");
  writeNmrProcessMarker(directory, "publisher-first-advance.marker");

  waitForNmrProcessMarker(
      directory, "requester-boundary-pending-observed.marker", options);
  auto const boundaryMessageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      10U);
  auto const boundaryTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      11U);
  std::vector<std::uint8_t> const boundaryPayload{0x42U, 0x4EU, 0x44U};
  std::vector<std::uint8_t> const boundaryTag{
      0x42U, 0x4EU, 0x44U, 0x2DU, 0x46U};
  sendTimestampedInteraction(
      *boundaryMessageTime,
      boundaryPayload,
      boundaryTag,
      "inclusive-boundary NMRA future TSO");
  writeNmrProcessMarker(directory, "publisher-boundary-tso-sent.marker");
  waitForNmrProcessMarker(
      directory, "requester-boundary-tso-quiet.marker", options);
  publisher.rtiAmbassador().timeAdvanceRequest(*boundaryTarget);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() >= 2U; },
      options,
      "process-separated NMRA publisher boundary grant");
  require(
      publisher.recorder().timeAdvanceGrants().size() == 2U &&
          publisher.recorder().timeAdvanceGrants().back().encoded ==
              encodeTime(*boundaryTarget),
      "process-separated NMRA publisher missed its boundary target");
  writeNmrProcessMarker(directory, "publisher-boundary-advance.marker");
  waitForNmrProcessMarker(directory, "requester-complete.marker", options);
  publisher.resign(rti::NO_ACTION);
  publisher.disconnect();
  writeNmrProcessMarker(directory, "publisher-resigned.marker");
}

}  // namespace

void scenarioNmraFutureTsoArrivalAfterPendingRequest(
    Options const& options,
    rti::CallbackModel model) {
  if (!options.nmrProcessRole.empty()) {
    auto const directory = nextNmrProcessRunDirectory(options);
    if (options.nmrProcessRole == "requester") {
      runNmraFutureArrivalRequesterProcess(options, model, directory);
      return;
    }
    if (options.nmrProcessRole == "publisher") {
      runNmraFutureArrivalPublisherProcess(options, model, directory);
      return;
    }
    throw std::runtime_error(
        "NMRA future-arrival process role must be requester or publisher");
  }
  require(
      options.nmrProcessCoordinationDirectory.empty(),
      "NMRA future-arrival coordination directory requires a process role");
  Session requester(options, model, "nmra-future-arrival-requester");
  Session peer(options, model, "nmra-future-arrival-peer");
  auto const federation = federationName(
      options, "nmra-future-tso-arrival-pending-request");
  connectAndJoin(requester, peer, options, federation, options.fom);

  auto const requesterInteraction = requester.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const peerInteraction = peer.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const requesterParameter = requester.rtiAmbassador().getParameterHandle(
      requesterInteraction, options.parameterName);
  auto const peerParameter = peer.rtiAmbassador().getParameterHandle(
      peerInteraction, options.parameterName);
  require(
      requesterInteraction.isValid() && peerInteraction.isValid() &&
          requesterParameter.isValid() && peerParameter.isValid() &&
          requesterInteraction == peerInteraction &&
          requesterParameter == peerParameter,
      "NMRA future-arrival FOM lookup returned inconsistent interaction handles");
  requester.rtiAmbassador().subscribeInteractionClass(requesterInteraction, true);
  peer.rtiAmbassador().publishInteractionClass(peerInteraction);
  peer.rtiAmbassador().changeInteractionOrderType(peerInteraction, rti::TIMESTAMP);

  auto requesterTime = makeTimeContext(requester);
  auto peerTime = makeTimeContext(peer);
  require(
      requesterTime.factory->getName() == peerTime.factory->getName(),
      "NMRA future-arrival members selected different logical-time factories");
  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMRA future-arrival requester Time Constrained callback");
  peer.rtiAmbassador().enableTimeRegulation(*peerTime.epsilon);
  waitFor(
      peer,
      [&] { return peer.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMRA future-arrival peer Time Regulation callback");
  requester.recorder().clearTimeCallbacks();
  peer.recorder().clearTimeCallbacks();
  requester.recorder().clearCallbackOrder();

  auto const messageTime = timeAfter(
      *peerTime.factory, *peerTime.initial, *peerTime.epsilon, 5U);
  auto const requestBoundary = timeAfter(
      *requesterTime.factory, *requesterTime.initial, *requesterTime.epsilon, 10U);
  auto const peerTarget = timeAfter(
      *peerTime.factory, *peerTime.initial, *peerTime.epsilon, 4U);

  // Exercise the process path in the order that used to be missing: the
  // request is pending before the peer's later TSO send reaches the RTI.
  requester.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  requester.pump();
  require(
      requester.recorder().timeAdvanceGrants().empty() &&
          requester.recorder().timedInteractions().empty(),
      "NMRA future-arrival requester completed before a qualifying TSO arrived");

  std::vector<std::uint8_t> const payloadBytes{0x4EU, 0x4DU, 0x52U, 0x41U};
  std::vector<std::uint8_t> const tagBytes{0x4EU, 0x4DU, 0x52U, 0x41U, 0x2DU, 0x46U};
  rti::ParameterHandleValueMap values;
  values.emplace(
      peerParameter,
      rti::VariableLengthData(payloadBytes.data(), payloadBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = peer.rtiAmbassador().sendInteraction(
      peerInteraction, values, tag, *messageTime);
  require(
      retraction.isValid(),
      "NMRA future-arrival peer send returned an invalid retraction handle");
  requester.pump();
  peer.pump();
  require(
      requester.recorder().timeAdvanceGrants().empty() &&
          requester.recorder().timedInteractions().empty(),
      "NMRA future-arrival requester crossed the regulator's time boundary");

  peer.rtiAmbassador().timeAdvanceRequest(*peerTarget);
  waitFor(
      peer,
      requester,
      [&] {
        return peer.recorder().timeAdvanceGrants().size() >= 1U &&
            requester.recorder().timedInteractions().size() >= 1U &&
            requester.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "NMRA future-arrival delivery and grant");

  require(
      peer.recorder().timeAdvanceGrants().size() == 1U &&
          peer.recorder().timeAdvanceGrants().front().encoded == encodeTime(*peerTarget),
      "NMRA future-arrival peer did not reach its requested time");
  require(
      requester.recorder().timedInteractions().size() == 1U &&
          requester.recorder().timeAdvanceGrants().size() == 1U,
      "NMRA future-arrival requester received duplicate or incomplete callbacks");
  auto const received = requester.recorder().timedInteractions().front();
  require(
      received.interaction == requesterInteraction &&
          received.parameters.size() == 1U &&
          received.parameters.count(requesterParameter) == 1U &&
          copyBytes(received.parameters.at(requesterParameter)) == payloadBytes &&
          received.tag == tagBytes && received.producer == peer.federateHandle(),
      "NMRA future-arrival callback changed the interaction payload or producer");
  require(
      received.time == encodeTime(*messageTime) &&
          received.sentOrder == rti::TIMESTAMP &&
          received.receivedOrder == rti::TIMESTAMP &&
          received.retractionPresent &&
          received.retraction == copyBytes(retraction.encode()),
      "NMRA future-arrival callback changed TSO timestamp or metadata");
  require(
      requester.recorder().timeAdvanceGrants().front().encoded == encodeTime(*messageTime) &&
          requester.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "grant"},
      "NMRA future-arrival grant did not follow delivery at the message timestamp");
  auto requesterCurrentTime = requesterTime.factory->makeInitial();
  require(
      requesterCurrentTime != nullptr,
      "NMRA future-arrival could not allocate a logical-time query value");
  requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
  requireTimeEquals(
      *requesterCurrentTime, *messageTime, "NMRA future-arrival requester time");

  // The request limit is inclusive. The peer is now at 4 with lookahead 1,
  // which keeps the later request to 10 pending while the boundary TSO at 10
  // arrives; the peer can then safely advance to 11.
  requester.recorder().clearTimeCallbacks();
  peer.recorder().clearTimeCallbacks();
  requester.recorder().clearCallbackOrder();
  auto const priorTimedInteractionCount =
      requester.recorder().timedInteractions().size();
  auto const boundaryMessageTime = timeAfter(
      *peerTime.factory, *peerTime.initial, *peerTime.epsilon, 10U);
  auto const boundaryPeerTarget = timeAfter(
      *peerTime.factory, *peerTime.initial, *peerTime.epsilon, 11U);
  requester.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  requester.pump();
  require(
      requester.recorder().timeAdvanceGrants().empty() &&
          requester.recorder().timedInteractions().size() ==
              priorTimedInteractionCount,
      "NMRA boundary requester completed before the boundary TSO arrived");

  std::vector<std::uint8_t> const boundaryPayloadBytes{0x42U, 0x4EU, 0x44U};
  std::vector<std::uint8_t> const boundaryTagBytes{0x42U, 0x4EU, 0x44U, 0x2DU, 0x46U};
  rti::ParameterHandleValueMap boundaryValues;
  boundaryValues.emplace(
      peerParameter,
      rti::VariableLengthData(
          boundaryPayloadBytes.data(), boundaryPayloadBytes.size()));
  rti::VariableLengthData const boundaryTag(
      boundaryTagBytes.data(), boundaryTagBytes.size());
  auto const boundaryRetraction = peer.rtiAmbassador().sendInteraction(
      peerInteraction, boundaryValues, boundaryTag, *boundaryMessageTime);
  require(
      boundaryRetraction.isValid(),
      "NMRA boundary peer send returned an invalid retraction handle");
  requester.pump();
  peer.pump();
  require(
      requester.recorder().timeAdvanceGrants().empty() &&
          requester.recorder().timedInteractions().size() ==
              priorTimedInteractionCount,
      "NMRA boundary requester crossed the regulator's time boundary");

  peer.rtiAmbassador().timeAdvanceRequest(*boundaryPeerTarget);
  waitFor(
      peer,
      requester,
      [&] {
        return peer.recorder().timeAdvanceGrants().size() >= 1U &&
            requester.recorder().timedInteractions().size() >=
                priorTimedInteractionCount + 1U &&
            requester.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "NMRA boundary future-arrival delivery and grant");
  require(
      peer.recorder().timeAdvanceGrants().size() == 1U &&
          peer.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*boundaryPeerTarget),
      "NMRA boundary peer did not reach its requested time");
  require(
      requester.recorder().timedInteractions().size() ==
              priorTimedInteractionCount + 1U &&
          requester.recorder().timeAdvanceGrants().size() == 1U,
      "NMRA boundary requester received duplicate or incomplete callbacks");
  auto const boundaryReceived = requester.recorder().timedInteractions().back();
  require(
      boundaryReceived.interaction == requesterInteraction &&
          boundaryReceived.parameters.size() == 1U &&
          boundaryReceived.parameters.count(requesterParameter) == 1U &&
          copyBytes(boundaryReceived.parameters.at(requesterParameter)) ==
              boundaryPayloadBytes &&
          boundaryReceived.tag == boundaryTagBytes &&
          boundaryReceived.producer == peer.federateHandle(),
      "NMRA boundary callback changed the interaction payload or producer");
  require(
      boundaryReceived.time == encodeTime(*boundaryMessageTime) &&
          boundaryReceived.sentOrder == rti::TIMESTAMP &&
          boundaryReceived.receivedOrder == rti::TIMESTAMP &&
          boundaryReceived.retractionPresent &&
          boundaryReceived.retraction == copyBytes(boundaryRetraction.encode()),
      "NMRA boundary callback changed TSO timestamp or metadata");
  require(
      requester.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*boundaryMessageTime) &&
          requester.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "grant"},
      "NMRA did not deliver a TSO at the inclusive request boundary before its grant");
  requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
  requireTimeEquals(
      *requesterCurrentTime,
      *boundaryMessageTime,
      "NMRA future-arrival requester time at the inclusive limit");

  requester.rtiAmbassador().disableTimeConstrained();
  requester.resign(rti::NO_ACTION);
  peer.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioNmraNoTsoAtGaltGrantTime(
    Options const& options,
    rti::CallbackModel model) {
  Session regulator(options, model, "nmra-galt-regulator");
  Session requester(options, model, "nmra-galt-requester");
  auto const federation = federationName(options, "nmra-no-tso-at-galt-grant-time");
  connectAndJoin(regulator, requester, options, federation, options.fom);

  auto regulatorTime = makeTimeContext(regulator);
  auto requesterTime = makeTimeContext(requester);
  require(
      regulatorTime.factory->getName() == requesterTime.factory->getName(),
      "NMRA GALT members selected different logical-time factories");
  requireTimeEquals(
      *regulatorTime.initial,
      *requesterTime.initial,
      "NMRA GALT initial logical time");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMRA GALT requester Time Constrained callback");

  auto const lookaheadTarget = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      5U);
  auto regulatorLookahead = regulatorTime.factory->makeZero();
  require(
      regulatorLookahead != nullptr,
      "NMRA GALT regulator could not allocate a lookahead interval");
  regulatorLookahead->setToDifference(*lookaheadTarget, *regulatorTime.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*regulatorLookahead);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMRA GALT regulator Time Regulation callback");

  auto expectedGalt = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      5U);
  auto observedGalt = requesterTime.factory->makeInitial();
  require(
      observedGalt != nullptr && requester.rtiAmbassador().queryGALT(*observedGalt),
      "NMRA requester could not observe the regulator's defined GALT");
  requireTimeEquals(*observedGalt, *expectedGalt, "NMRA initial GALT");

  requester.rtiAmbassador().nextMessageRequestAvailable(*expectedGalt);
  waitFor(
      requester,
      [&] { return requester.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "NMRA no-TSO grant at GALT request time");
  require(
      requester.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*expectedGalt) &&
          regulator.recorder().timeAdvanceGrants().empty(),
      "NMRA with no TSO did not grant at its request time without advancing the peer");
  auto regulatorCurrentTime = regulatorTime.factory->makeInitial();
  auto requesterCurrentTime = requesterTime.factory->makeInitial();
  require(
      regulatorCurrentTime != nullptr && requesterCurrentTime != nullptr,
      "NMRA GALT test could not allocate queried logical times");
  regulator.rtiAmbassador().queryLogicalTime(*regulatorCurrentTime);
  requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
  requireTimeEquals(
      *regulatorCurrentTime,
      *regulatorTime.initial,
      "NMRA no-TSO grant changed regulator time");
  requireTimeEquals(
      *requesterCurrentTime,
      *expectedGalt,
      "NMRA no-TSO grant did not set requester time to the request time");
  require(
      requester.rtiAmbassador().queryGALT(*observedGalt),
      "NMRA no-TSO grant left GALT undefined");
  requireTimeEquals(*observedGalt, *expectedGalt, "NMRA no-TSO grant at GALT");
  require(
      requester.recorder().timeAdvanceGrants().size() == 1U &&
          requester.recorder().timedInteractions().empty(),
      "NMRA no-TSO grant produced duplicate grants or an unexpected TSO");

  requester.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  regulator.rtiAmbassador().destroyFederationExecution(federation);
}

namespace {

void runNmraNoTsoRequesterProcess(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& directory) {
  Session requester(options, model, "owner");
  requester.connect();
  auto const federation = federationName(options, "nmra-no-tso-waits-for-galt");
  requester.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  writeNmrProcessMarker(directory, "federation-created.marker");
  requester.join(options.ownerFederateName, options.federateType, federation);

  auto requesterTime = makeTimeContext(requester);
  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "process-separated NMRA requester Time Constrained callback");
  waitForNmrProcessMarker(directory, "regulator-ready.marker", options);

  auto const initialGalt = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      1U);
  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      5U);
  auto const firstAdvancedGalt = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      4U);
  auto const requestTimeGalt = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      5U);
  auto observedGalt = requesterTime.factory->makeInitial();
  require(
      observedGalt != nullptr && requester.rtiAmbassador().queryGALT(*observedGalt),
      "process-separated NMRA requester could not query initial GALT");
  requireTimeEquals(*observedGalt, *initialGalt, "process-separated initial GALT");

  requester.rtiAmbassador().nextMessageRequestAvailable(*requestTime);
  writeNmrProcessMarker(directory, "requester-pending.marker");
  auto requireNoGrantAtGalt = [&](rti::LogicalTime const& expected,
                                  std::string const& context) {
    auto const observationMilliseconds =
        std::max(1, std::min(options.timeoutMilliseconds, 250));
    auto const quietDeadline = Clock::now() +
        std::chrono::milliseconds(observationMilliseconds);
    while (Clock::now() < quietDeadline) {
      requester.pump();
      require(
          requester.recorder().timeAdvanceGrants().empty() &&
              requester.recorder().timedInteractions().empty(),
          context);
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(
        requester.rtiAmbassador().queryGALT(*observedGalt),
        context + ": queryGALT failed");
    requireTimeEquals(*observedGalt, expected, context + ": unexpected GALT");
  };
  requireNoGrantAtGalt(
      *initialGalt,
      "process-separated NMRA granted before the regulator advanced");
  writeNmrProcessMarker(directory, "requester-observed-initial-galt.marker");

  waitForNmrProcessMarker(directory, "regulator-first-advance.marker", options);
  requireNoGrantAtGalt(
      *firstAdvancedGalt,
      "process-separated NMRA granted while GALT remained below the request");
  writeNmrProcessMarker(directory, "requester-observed-intermediate-galt.marker");

  waitForNmrProcessMarker(directory, "regulator-second-advance.marker", options);
  waitFor(
      requester,
      [&] { return requester.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "process-separated NMRA grant when GALT reaches the request");
  require(
      requester.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*requestTime) &&
          requester.recorder().timedInteractions().empty(),
      "process-separated NMRA did not grant at request time without a TSO");
  require(
      requester.rtiAmbassador().queryGALT(*observedGalt),
      "process-separated NMRA requester could not query request-time GALT");
  requireTimeEquals(*observedGalt, *requestTimeGalt, "process-separated request-time GALT");
  writeNmrProcessMarker(directory, "requester-granted.marker");

  waitForNmrProcessMarker(directory, "regulator-resigned.marker", options);
  requester.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
}

void runNmraNoTsoRegulatorProcess(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& directory) {
  waitForNmrProcessMarker(directory, "federation-created.marker", options);
  Session regulator(options, model, "member");
  regulator.connect();
  auto const federation = federationName(options, "nmra-no-tso-waits-for-galt");
  regulator.join(options.memberFederateName, options.federateType, federation);

  auto regulatorTime = makeTimeContext(regulator);
  auto const initialGalt = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      1U);
  auto lookahead = regulatorTime.factory->makeZero();
  require(lookahead != nullptr, "process-separated NMRA regulator lacks lookahead");
  lookahead->setToDifference(*initialGalt, *regulatorTime.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "process-separated NMRA regulator Time Regulation callback");
  writeNmrProcessMarker(directory, "regulator-ready.marker");
  waitForNmrProcessMarker(directory, "requester-pending.marker", options);
  waitForNmrProcessMarker(directory, "requester-observed-initial-galt.marker", options);

  auto const firstTarget = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      3U);
  auto const secondTarget = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      4U);
  regulator.rtiAmbassador().timeAdvanceRequest(*firstTarget);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "process-separated NMRA regulator first grant");
  require(
      regulator.recorder().timeAdvanceGrants().front().encoded == encodeTime(*firstTarget),
      "process-separated NMRA regulator missed its first target");
  writeNmrProcessMarker(directory, "regulator-first-advance.marker");
  waitForNmrProcessMarker(directory, "requester-observed-intermediate-galt.marker", options);

  regulator.rtiAmbassador().timeAdvanceRequest(*secondTarget);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "process-separated NMRA regulator second grant");
  require(
      regulator.recorder().timeAdvanceGrants().back().encoded == encodeTime(*secondTarget),
      "process-separated NMRA regulator missed its second target");
  writeNmrProcessMarker(directory, "regulator-second-advance.marker");
  waitForNmrProcessMarker(directory, "requester-granted.marker", options);
  regulator.resign(rti::NO_ACTION);
  regulator.disconnect();
  writeNmrProcessMarker(directory, "regulator-resigned.marker");
}

}  // namespace

void scenarioNmraNoTsoWaitsForGalt(
    Options const& options,
    rti::CallbackModel model) {
  if (!options.nmrProcessRole.empty()) {
    auto const directory = nextNmrProcessRunDirectory(options);
    if (options.nmrProcessRole == "requester") {
      runNmraNoTsoRequesterProcess(options, model, directory);
      return;
    }
    if (options.nmrProcessRole == "publisher") {
      runNmraNoTsoRegulatorProcess(options, model, directory);
      return;
    }
    throw std::runtime_error(
        "NMRA no-TSO process role must be requester or publisher");
  }
  require(
      options.nmrProcessCoordinationDirectory.empty(),
      "NMRA no-TSO coordination directory requires a process role");

  Session regulator(options, model, "nmra-galt-wait-regulator");
  Session requester(options, model, "nmra-galt-wait-requester");
  auto const federation = federationName(options, "nmra-no-tso-waits-for-galt");
  connectAndJoin(regulator, requester, options, federation, options.fom);

  auto regulatorTime = makeTimeContext(regulator);
  auto requesterTime = makeTimeContext(requester);
  require(
      regulatorTime.factory->getName() == requesterTime.factory->getName(),
      "NMRA GALT-wait members selected different logical-time factories");
  requireTimeEquals(
      *regulatorTime.initial,
      *requesterTime.initial,
      "NMRA GALT-wait initial logical time");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMRA GALT-wait requester Time Constrained callback");

  auto const initialGalt = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      1U);
  auto regulatorLookahead = regulatorTime.factory->makeZero();
  require(
      regulatorLookahead != nullptr,
      "NMRA GALT-wait regulator could not allocate a lookahead interval");
  regulatorLookahead->setToDifference(*initialGalt, *regulatorTime.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*regulatorLookahead);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMRA GALT-wait regulator Time Regulation callback");

  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      5U);
  auto const regulatorFirstTarget = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      3U);
  auto const galtAfterFirstAdvance = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      4U);
  auto const regulatorSecondTarget = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      4U);
  auto expectedGalt = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      5U);
  auto observedGalt = requesterTime.factory->makeInitial();
  require(
      observedGalt != nullptr && requester.rtiAmbassador().queryGALT(*observedGalt),
      "NMRA GALT-wait requester could not observe initial GALT");
  requireTimeEquals(*observedGalt, *initialGalt, "NMRA initial GALT below request");

  requester.rtiAmbassador().nextMessageRequestAvailable(*requestTime);
  auto requireNoPrematureGrant = [&](std::string const& description) {
    auto const observationMilliseconds =
        std::max(1, std::min(options.timeoutMilliseconds, 250));
    auto const quietDeadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(observationMilliseconds);
    while (std::chrono::steady_clock::now() < quietDeadline) {
      requester.pump();
      regulator.pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(
        requester.recorder().timeAdvanceGrants().empty() &&
            requester.recorder().timedInteractions().empty(),
        description);
  };
  requireNoPrematureGrant(
      "NMRA without TSO granted while initial GALT was below the request");

  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorFirstTarget);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "NMRA GALT-wait regulator first advance");
  require(
      regulator.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*regulatorFirstTarget),
      "NMRA GALT-wait regulator did not reach its first target");
  require(
      requester.rtiAmbassador().queryGALT(*observedGalt),
      "NMRA GALT-wait requester could not observe intermediate GALT");
  requireTimeEquals(
      *observedGalt,
      *galtAfterFirstAdvance,
      "NMRA intermediate GALT below request");
  requireNoPrematureGrant(
      "NMRA without TSO granted while intermediate GALT remained below the request");

  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorSecondTarget);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "NMRA GALT-wait regulator second advance");
  require(
      regulator.recorder().timeAdvanceGrants().back().encoded ==
          encodeTime(*regulatorSecondTarget),
      "NMRA GALT-wait regulator did not reach its second target");
  require(
      requester.rtiAmbassador().queryGALT(*observedGalt),
      "NMRA GALT-wait requester could not observe request-time GALT");
  requireTimeEquals(
      *observedGalt,
      *expectedGalt,
      "NMRA request-time GALT after regulator advance");

  waitFor(
      requester,
      [&] { return requester.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "NMRA no-TSO grant when GALT reaches request time");
  require(
      requester.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*requestTime) &&
          requester.recorder().timedInteractions().empty(),
      "NMRA without TSO did not grant at request time when GALT reached it");

  auto regulatorCurrentTime = regulatorTime.factory->makeInitial();
  auto requesterCurrentTime = requesterTime.factory->makeInitial();
  require(
      regulatorCurrentTime != nullptr && requesterCurrentTime != nullptr,
      "NMRA GALT-wait test could not allocate queried logical times");
  regulator.rtiAmbassador().queryLogicalTime(*regulatorCurrentTime);
  requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
  requireTimeEquals(
      *regulatorCurrentTime,
      *regulatorSecondTarget,
      "NMRA GALT-wait regulator current time");
  requireTimeEquals(
      *requesterCurrentTime,
      *requestTime,
      "NMRA GALT-wait requester current time");
  require(
      requester.recorder().timeAdvanceGrants().size() == 1U &&
          requester.recorder().timedInteractions().empty(),
      "NMRA GALT-wait produced duplicate grants or an unexpected TSO");

  requester.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  regulator.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioNmraNoTsoAtGaltGrantTimeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraNoTsoAtGaltGrantTime(options, model);
}

void scenarioNmraNoTsoWaitsForGaltContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraNoTsoWaitsForGalt(options, model);
}

namespace {

void runNoTsoUndefinedGaltNmra(
    Options const& options,
    rti::CallbackModel model,
    bool expectedNrgEnabled) {
  auto const switchName = expectedNrgEnabled ? "enabled" : "disabled";
  Session regulator(options, model, "nmra-no-tso-nrg-regulator");
  Session secondRegulator(options, model, "nmra-no-tso-nrg-second-regulator");
  Session requester(options, model, "nmra-no-tso-nrg-requester");
  auto const federation = federationName(
      options, std::string("nmra-no-tso-undefined-galt-nrg-") + switchName);
  regulator.connect();
  secondRegulator.connect();
  requester.connect();
  regulator.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  regulator.join(options.ownerFederateName, options.federateType, federation);
  secondRegulator.join(options.ownerFederateName + L"-second-regulator",
                       options.federateType, federation);
  requester.join(options.memberFederateName, options.federateType, federation);

  auto const nrgEnabled = regulator.rtiAmbassador().getNonRegulatedGrantSwitch();
  require(nrgEnabled == expectedNrgEnabled,
          "NMRA no-TSO adapter FOM did not match its expected NRG switch");
  require(requester.rtiAmbassador().getNonRegulatedGrantSwitch() == nrgEnabled,
          "NMRA no-TSO federates disagreed on the NRG switch");
  require(secondRegulator.rtiAmbassador().getNonRegulatedGrantSwitch() == nrgEnabled,
          "NMRA no-TSO regulators disagreed on the NRG switch");

  auto regulatorTime = makeTimeContext(regulator);
  auto secondRegulatorTime = makeTimeContext(secondRegulator);
  auto requesterTime = makeTimeContext(requester);
  require(regulatorTime.factory->getName() == requesterTime.factory->getName() &&
              secondRegulatorTime.factory->getName() ==
                  requesterTime.factory->getName(),
          "NMRA no-TSO members selected different logical-time factories");
  requireTimeEquals(*regulatorTime.initial, *requesterTime.initial,
                    "NMRA no-TSO initial logical time");
  requireTimeEquals(*secondRegulatorTime.initial, *requesterTime.initial,
                    "NMRA no-TSO second regulator initial logical time");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(requester,
          [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "NMRA no-TSO requester Time Constrained callback");

  auto const initialGalt = timeAfter(
      *regulatorTime.factory, *regulatorTime.initial,
      *regulatorTime.epsilon, 1U);
  auto const primaryRegulatorInitialGalt = timeAfter(
      *regulatorTime.factory, *regulatorTime.initial,
      *regulatorTime.epsilon, 2U);
  auto primaryRegulatorLookahead = regulatorTime.factory->makeZero();
  auto secondRegulatorLookahead = secondRegulatorTime.factory->makeZero();
  require(primaryRegulatorLookahead != nullptr &&
              secondRegulatorLookahead != nullptr,
          "NMRA no-TSO regulators could not allocate lookahead intervals");
  primaryRegulatorLookahead->setToDifference(
      *primaryRegulatorInitialGalt, *regulatorTime.initial);
  secondRegulatorLookahead->setToDifference(
      *initialGalt, *secondRegulatorTime.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*primaryRegulatorLookahead);
  waitFor(regulator,
          [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
          options, "NMRA no-TSO primary regulator Time Regulation callback");
  secondRegulator.rtiAmbassador().enableTimeRegulation(*secondRegulatorLookahead);
  waitFor(secondRegulator,
          [&] {
            return secondRegulator.recorder().timeRegulationEnabled().size() ==
                1U;
          },
          options, "NMRA no-TSO second regulator Time Regulation callback");

  auto galt = requesterTime.factory->makeInitial();
  require(galt != nullptr && requester.rtiAmbassador().queryGALT(*galt),
          "NMRA no-TSO setup did not establish a defined GALT");
  requireTimeEquals(*galt, *initialGalt, "NMRA no-TSO initial GALT");

  auto const requestTime = timeAfter(
      *requesterTime.factory, *requesterTime.initial,
      *requesterTime.epsilon, 10U);
  regulator.rtiAmbassador().disableTimeRegulation();
  secondRegulator.rtiAmbassador().disableTimeRegulation();
  regulator.pump();
  secondRegulator.pump();
  require(!requester.rtiAmbassador().queryGALT(*galt),
          "NMRA no-TSO setup unexpectedly retained a defined GALT");
  requester.rtiAmbassador().nextMessageRequestAvailable(*requestTime);

  if (!expectedNrgEnabled) {
    auto const observationMilliseconds =
        std::max(1, std::min(options.timeoutMilliseconds, 250));
    auto const quietDeadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(observationMilliseconds);
    while (std::chrono::steady_clock::now() < quietDeadline) {
      requester.pump();
      regulator.pump();
      secondRegulator.pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(requester.recorder().timeAdvanceGrants().empty() &&
                requester.recorder().timedInteractions().empty(),
            "NMRA with undefined GALT and NRG disabled advanced without a regulator");
    auto requesterCurrentTime = requesterTime.factory->makeInitial();
    require(requesterCurrentTime != nullptr,
            "NMRA no-TSO could not allocate its current logical-time query value");
    requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
    requireTimeEquals(*requesterCurrentTime, *requesterTime.initial,
                      "NMRA no-TSO NRG-disabled time before regulator resumes");

    regulator.rtiAmbassador().enableTimeRegulation(*primaryRegulatorLookahead);
    waitFor(regulator,
            [&] { return regulator.recorder().timeRegulationEnabled().size() == 2U; },
            options, "NMRA no-TSO resumed primary regulator Time Regulation callback");
    secondRegulator.rtiAmbassador().enableTimeRegulation(*secondRegulatorLookahead);
    waitFor(secondRegulator,
            [&] {
              return secondRegulator.recorder().timeRegulationEnabled().size() ==
                  2U;
            },
            options,
            "NMRA no-TSO resumed second regulator Time Regulation callback");
    auto const regulatorTarget = timeAfter(
        *regulatorTime.factory, *regulatorTime.initial,
        *regulatorTime.epsilon, 11U);
    regulator.rtiAmbassador().timeAdvanceRequest(*regulatorTarget);
    waitFor(regulator,
            [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
            options, "NMRA no-TSO resumed regulator advance");
    require(regulator.recorder().timeAdvanceGrants().front().encoded ==
                encodeTime(*regulatorTarget),
            "NMRA no-TSO resumed regulator missed its requested time");

    auto const secondRegulatorQuietDeadline =
        std::chrono::steady_clock::now() +
        std::chrono::milliseconds(observationMilliseconds);
    while (std::chrono::steady_clock::now() < secondRegulatorQuietDeadline) {
      requester.pump();
      secondRegulator.pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(requester.recorder().timeAdvanceGrants().empty() &&
                requester.recorder().timedInteractions().empty(),
            "NMRA granted while the second regulator still limited GALT");
    auto requesterCurrentTimeAfterFirstRegulator =
        requesterTime.factory->makeInitial();
    require(requesterCurrentTimeAfterFirstRegulator != nullptr,
            "NMRA no-TSO could not allocate its first-regulator time query");
    requester.rtiAmbassador().queryLogicalTime(
        *requesterCurrentTimeAfterFirstRegulator);
    requireTimeEquals(*requesterCurrentTimeAfterFirstRegulator,
                      *requesterTime.initial,
                      "NMRA no-TSO time after only one regulator advanced");
    auto const galtWhileSecondRegulatorIsBehind =
        requesterTime.factory->makeInitial();
    require(galtWhileSecondRegulatorIsBehind != nullptr &&
                requester.rtiAmbassador().queryGALT(
                    *galtWhileSecondRegulatorIsBehind),
            "NMRA no-TSO lost GALT while the second regulator remained active");
    requireTimeEquals(*galtWhileSecondRegulatorIsBehind, *initialGalt,
                      "NMRA no-TSO GALT after only the primary regulator advanced");

    auto const secondRegulatorTarget = timeAfter(
        *secondRegulatorTime.factory,
        *secondRegulatorTime.initial,
        *secondRegulatorTime.epsilon,
        11U);
    secondRegulator.rtiAmbassador().timeAdvanceRequest(*secondRegulatorTarget);
    waitFor(secondRegulator,
            [&] {
              return secondRegulator.recorder().timeAdvanceGrants().size() ==
                  1U;
            },
            options, "NMRA no-TSO second regulator advance");
    require(secondRegulator.recorder().timeAdvanceGrants().front().encoded ==
                encodeTime(*secondRegulatorTarget),
            "NMRA no-TSO second regulator missed its requested time");
    auto const galtAfterBothRegulatorsAdvance =
        requesterTime.factory->makeInitial();
    require(galtAfterBothRegulatorsAdvance != nullptr &&
                requester.rtiAmbassador().queryGALT(
                    *galtAfterBothRegulatorsAdvance),
            "NMRA no-TSO lost GALT after both regulators advanced");
    require(*galtAfterBothRegulatorsAdvance > *requestTime,
            "NMRA no-TSO GALT did not clear the request after both regulators advanced");
  }

  waitFor(requester,
          [&] { return requester.recorder().timeAdvanceGrants().size() == 1U; },
          options, "NMRA no-TSO grant at the requested time");
  require(requester.recorder().timedInteractions().empty(),
          "NMRA no-TSO grant unexpectedly delivered a TSO");
  auto const grants = requester.recorder().timeAdvanceGrants();
  require(grants.size() == 1U && grants.front().encoded == encodeTime(*requestTime),
          "NMRA no-TSO did not grant exactly at its request limit");
  require(requester.recorder().callbackOrder() == std::vector<std::string>{"grant"},
          "NMRA no-TSO produced an unexpected callback order");
  require(regulator.recorder().timeAdvanceGrants().size() ==
              (expectedNrgEnabled ? 0U : 1U),
          "NMRA no-TSO used an unexpected number of primary regulator advances");
  require(secondRegulator.recorder().timeAdvanceGrants().size() ==
              (expectedNrgEnabled ? 0U : 1U),
          "NMRA no-TSO used an unexpected number of second regulator advances");

  auto requesterCurrentTime = requesterTime.factory->makeInitial();
  require(requesterCurrentTime != nullptr,
          "NMRA no-TSO could not allocate its final logical-time query value");
  requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
  requireTimeEquals(*requesterCurrentTime, *requestTime,
                    "NMRA no-TSO final requester time");
  if (expectedNrgEnabled) {
    require(!requester.rtiAmbassador().queryGALT(*galt),
            "NMRA no-TSO NRG-enabled grant unexpectedly defined GALT");
  }

  requester.rtiAmbassador().disableTimeConstrained();
  requester.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  secondRegulator.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
}

void runNoTsoUndefinedGaltNmraMatrix(
    Options const& options,
    rti::CallbackModel model) {
  auto const disabledFom = options.scenarioFoms.find("nmra-undefined-galt-nrg-disabled");
  auto const enabledFom = options.scenarioFoms.find("nmra-undefined-galt-nrg-enabled");
  require(disabledFom != options.scenarioFoms.end() && !disabledFom->second.empty() &&
              enabledFom != options.scenarioFoms.end() && !enabledFom->second.empty(),
          "NMRA no-TSO matrix requires NRG-disabled and NRG-enabled adapter FOMs");
  auto disabledOptions = options;
  disabledOptions.fom = disabledFom->second;
  runNoTsoUndefinedGaltNmra(disabledOptions, model, false);
  auto enabledOptions = options;
  enabledOptions.fom = enabledFom->second;
  runNoTsoUndefinedGaltNmra(enabledOptions, model, true);
}

}  // namespace

void scenarioNmraNoTsoUndefinedGaltNrg(
    Options const& options,
    rti::CallbackModel model) {
  runNoTsoUndefinedGaltNmraMatrix(options, model);
}

void scenarioNmraNoTsoUndefinedGaltNrgContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraNoTsoUndefinedGaltNrg(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
