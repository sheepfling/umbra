#include "tck_main_scenarios.hpp"
#include "tck_nmr_process_coordination.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

void runUndefinedGaltTara(
    Options const& options,
    rti::CallbackModel model,
    bool expectedNrgEnabled) {
  auto const switchName = expectedNrgEnabled ? "enabled" : "disabled";
  Session regulator(options, model, "tara-nrg-regulator");
  Session requester(options, model, "tara-nrg-requester");
  auto const federation = federationName(
      options,
      std::string("tara-undefined-galt-nrg-") + switchName);
  connectAndJoin(regulator, requester, options, federation, options.fom);

  auto const nrgEnabled = regulator.rtiAmbassador().getNonRegulatedGrantSwitch();
  require(nrgEnabled == expectedNrgEnabled,
          "TARA adapter FOM did not match its expected NRG switch");
  require(requester.rtiAmbassador().getNonRegulatedGrantSwitch() == nrgEnabled,
          "TARA federates disagreed on the NRG switch");

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
          "TARA undefined-GALT FOM did not resolve consistent interaction handles");
  regulator.rtiAmbassador().publishInteractionClass(regulatorInteraction);
  regulator.rtiAmbassador().changeInteractionOrderType(
      regulatorInteraction, rti::TIMESTAMP);
  requester.rtiAmbassador().subscribeInteractionClass(requesterInteraction, true);

  auto regulatorTime = makeTimeContext(regulator);
  auto requesterTime = makeTimeContext(requester);
  require(regulatorTime.factory->getName() == requesterTime.factory->getName(),
          "TARA undefined-GALT members selected different logical-time factories");
  requireTimeEquals(*regulatorTime.initial, *requesterTime.initial,
                    "TARA undefined-GALT initial logical time");
  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(requester,
          [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "TARA undefined-GALT requester Time Constrained callback");

  auto const lookahead = regulatorTime.factory->makeZero();
  require(lookahead != nullptr, "TARA undefined-GALT could not allocate lookahead");
  lookahead->setToDifference(
      *timeAfter(*regulatorTime.factory, *regulatorTime.initial,
                 *regulatorTime.epsilon, 1U),
      *regulatorTime.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(regulator,
          [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
          options, "TARA undefined-GALT regulator Time Regulation callback");

  auto const messageTime = timeAfter(
      *regulatorTime.factory, *regulatorTime.initial,
      *regulatorTime.epsilon, 3U);
  auto const regulatorTarget = timeAfter(
      *regulatorTime.factory, *regulatorTime.initial,
      *regulatorTime.epsilon, 4U);
  auto const requestTime = timeAfter(
      *requesterTime.factory, *requesterTime.initial,
      *requesterTime.epsilon, 10U);
  std::vector<std::uint8_t> const payloadBytes{0x54U, 0x41U, 0x52U};
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x41U, 0x52U, 0x41U};
  rti::ParameterHandleValueMap values;
  values.emplace(regulatorParameter,
                 rti::VariableLengthData(payloadBytes.data(), payloadBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = regulator.rtiAmbassador().sendInteraction(
      regulatorInteraction, values, tag, *messageTime);
  require(retraction.isValid(),
          "TARA undefined-GALT send returned an invalid retraction handle");
  regulator.rtiAmbassador().disableTimeRegulation();
  regulator.pump();

  auto galt = requesterTime.factory->makeInitial();
  require(galt != nullptr && !requester.rtiAmbassador().queryGALT(*galt),
          "TARA undefined-GALT setup unexpectedly had a defined GALT");
  require(requester.recorder().timedInteractions().empty() &&
              requester.recorder().timeAdvanceGrants().empty(),
          "TARA undefined-GALT TSO was delivered before the request");
  requester.rtiAmbassador().timeAdvanceRequestAvailable(*requestTime);

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
            "TARA with undefined GALT and NRG disabled advanced beyond current time");
    auto requesterCurrentTime = requesterTime.factory->makeInitial();
    require(requesterCurrentTime != nullptr,
            "TARA undefined-GALT could not allocate a logical-time query value");
    requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
    requireTimeEquals(*requesterCurrentTime, *requesterTime.initial,
                      "TARA with NRG disabled before regulator resumes");

    regulator.rtiAmbassador().enableTimeRegulation(*lookahead);
    waitFor(regulator,
            [&] { return regulator.recorder().timeRegulationEnabled().size() == 2U; },
            options, "TARA resumed regulator Time Regulation callback");
    regulator.rtiAmbassador().timeAdvanceRequest(*regulatorTarget);
    waitFor(regulator,
            [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
            options, "TARA resumed regulator advance");
    require(regulator.recorder().timeAdvanceGrants().front().encoded ==
                encodeTime(*regulatorTarget),
            "TARA resumed regulator did not reach its requested time");
  }

  waitFor(requester,
          [&] {
            return requester.recorder().timedInteractions().size() == 1U &&
                requester.recorder().timeAdvanceGrants().size() == 1U;
          },
          options, "TARA undefined-GALT delivery and grant");
  auto const received = requester.recorder().timedInteractions().front();
  require(received.interaction == requesterInteraction &&
              received.parameters.size() == 1U &&
              received.parameters.count(requesterParameter) == 1U &&
              copyBytes(received.parameters.at(requesterParameter)) == payloadBytes &&
              received.tag == tagBytes && received.producer == regulator.federateHandle(),
          "TARA undefined-GALT delivered the wrong interaction payload or producer");
  require(received.time == encodeTime(*messageTime) &&
              received.sentOrder == rti::TIMESTAMP &&
              received.receivedOrder == rti::TIMESTAMP,
          "TARA undefined-GALT changed queued TSO time or order metadata");
  require(requester.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "grant"},
          "TARA undefined-GALT granted before delivering its queued TSO");
  auto const grants = requester.recorder().timeAdvanceGrants();
  require(grants.size() == 1U && grants.front().encoded == encodeTime(*messageTime),
          "TARA undefined-GALT did not grant at the queued TSO timestamp");
  require(regulator.recorder().timeAdvanceGrants().size() ==
              (expectedNrgEnabled ? 0U : 1U),
          "TARA undefined-GALT used the wrong number of regulator advances");

  auto requesterCurrentTime = requesterTime.factory->makeInitial();
  require(requesterCurrentTime != nullptr,
          "TARA undefined-GALT could not allocate the final logical-time query value");
  requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
  requireTimeEquals(*requesterCurrentTime, *messageTime,
                    "TARA undefined-GALT requester time at its queued TSO");
  requester.rtiAmbassador().disableTimeConstrained();
  requester.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
}

void runUndefinedGaltTaraMatrix(
    Options const& options,
    rti::CallbackModel model) {
  auto const disabledFom = options.scenarioFoms.find("tara-undefined-galt-nrg-disabled");
  auto const enabledFom = options.scenarioFoms.find("tara-undefined-galt-nrg-enabled");
  require(disabledFom != options.scenarioFoms.end() && !disabledFom->second.empty() &&
              enabledFom != options.scenarioFoms.end() && !enabledFom->second.empty(),
          "TARA undefined-GALT matrix requires NRG-disabled and NRG-enabled adapter FOMs");
  auto disabledOptions = options;
  disabledOptions.fom = disabledFom->second;
  runUndefinedGaltTara(disabledOptions, model, false);
  auto enabledOptions = options;
  enabledOptions.fom = enabledFom->second;
  runUndefinedGaltTara(enabledOptions, model, true);
}

void runNoTsoUndefinedGaltTara(
    Options const& options,
    rti::CallbackModel model,
    bool expectedNrgEnabled) {
  require(!options.logicalTimeImplementationName.empty(),
          "No-TSO undefined-GALT TARA requires adapter logical time");
  auto const switchName = expectedNrgEnabled ? "enabled" : "disabled";
  Session regulator(options, model, "tara-no-tso-regulator");
  Session secondRegulator(options, model, "tara-no-tso-second-regulator");
  Session requester(options, model, "tara-no-tso-requester");
  auto const federation = federationName(
      options,
      std::string("tara-no-tso-undefined-galt-") + switchName);
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
          "No-TSO TARA adapter FOM did not match its expected NRG switch");
  require(requester.rtiAmbassador().getNonRegulatedGrantSwitch() == nrgEnabled,
          "No-TSO TARA federates disagreed on the NRG switch");
  require(secondRegulator.rtiAmbassador().getNonRegulatedGrantSwitch() == nrgEnabled,
          "No-TSO TARA regulators disagreed on the NRG switch");

  auto regulatorTime = makeTimeContext(regulator);
  auto secondRegulatorTime = makeTimeContext(secondRegulator);
  auto requesterTime = makeTimeContext(requester);
  require(regulatorTime.factory->getName() == requesterTime.factory->getName() &&
              secondRegulatorTime.factory->getName() ==
                  requesterTime.factory->getName(),
          "No-TSO TARA members selected different logical-time factories");
  requireTimeEquals(*regulatorTime.initial, *requesterTime.initial,
                    "No-TSO TARA initial logical time");
  requireTimeEquals(*secondRegulatorTime.initial, *requesterTime.initial,
                    "No-TSO TARA second regulator initial logical time");
  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(requester,
          [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "No-TSO TARA requester Time Constrained callback");

  auto const initialGalt = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      1U);
  auto const primaryRegulatorGalt = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      2U);
  auto primaryRegulatorLookahead = regulatorTime.factory->makeZero();
  auto secondRegulatorLookahead = secondRegulatorTime.factory->makeZero();
  require(primaryRegulatorLookahead != nullptr &&
              secondRegulatorLookahead != nullptr,
          "No-TSO TARA regulators could not allocate lookahead intervals");
  primaryRegulatorLookahead->setToDifference(
      *primaryRegulatorGalt, *regulatorTime.initial);
  secondRegulatorLookahead->setToDifference(
      *initialGalt, *secondRegulatorTime.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*primaryRegulatorLookahead);
  waitFor(regulator,
          [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
          options, "No-TSO TARA primary regulator Time Regulation callback");
  secondRegulator.rtiAmbassador().enableTimeRegulation(*secondRegulatorLookahead);
  waitFor(secondRegulator,
          [&] {
            return secondRegulator.recorder().timeRegulationEnabled().size() ==
                1U;
          },
          options, "No-TSO TARA second regulator Time Regulation callback");
  regulator.rtiAmbassador().disableTimeRegulation();
  secondRegulator.rtiAmbassador().disableTimeRegulation();
  regulator.pump();
  secondRegulator.pump();

  auto galt = requesterTime.factory->makeInitial();
  require(galt != nullptr && !requester.rtiAmbassador().queryGALT(*galt),
          "No-TSO TARA setup unexpectedly had a defined GALT");
  require(requester.recorder().timedInteractions().empty() &&
              requester.recorder().timeAdvanceGrants().empty(),
          "No-TSO TARA setup unexpectedly delivered a TSO or grant");

  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  requester.rtiAmbassador().timeAdvanceRequestAvailable(*requestTime);

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
    require(requester.recorder().timedInteractions().empty() &&
                requester.recorder().timeAdvanceGrants().empty(),
            "No-TSO TARA with NRG disabled granted while GALT was undefined");
    auto requesterCurrentTime = requesterTime.factory->makeInitial();
    require(requesterCurrentTime != nullptr,
            "No-TSO TARA could not allocate a logical-time query value");
    requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
    requireTimeEquals(*requesterCurrentTime, *requesterTime.initial,
                      "No-TSO TARA NRG-disabled time before regulator advance");

    regulator.rtiAmbassador().enableTimeRegulation(*primaryRegulatorLookahead);
    waitFor(regulator,
            [&] { return regulator.recorder().timeRegulationEnabled().size() == 2U; },
            options,
            "No-TSO TARA resumed primary regulator Time Regulation callback");
    secondRegulator.rtiAmbassador().enableTimeRegulation(*secondRegulatorLookahead);
    waitFor(secondRegulator,
            [&] {
              return secondRegulator.recorder().timeRegulationEnabled().size() ==
                  2U;
            },
            options,
            "No-TSO TARA resumed second regulator Time Regulation callback");
    auto const regulatorTarget = timeAfter(
        *regulatorTime.factory,
        *regulatorTime.initial,
        *regulatorTime.epsilon,
        11U);
    regulator.rtiAmbassador().timeAdvanceRequest(*regulatorTarget);
    waitFor(regulator,
            [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
            options, "No-TSO TARA resumed regulator advance");
    require(regulator.recorder().timeAdvanceGrants().front().encoded ==
                encodeTime(*regulatorTarget),
            "No-TSO TARA regulator did not reach its requested time");

    auto const secondRegulatorQuietDeadline =
        std::chrono::steady_clock::now() +
        std::chrono::milliseconds(observationMilliseconds);
    while (std::chrono::steady_clock::now() < secondRegulatorQuietDeadline) {
      requester.pump();
      secondRegulator.pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(requester.recorder().timedInteractions().empty() &&
                requester.recorder().timeAdvanceGrants().empty(),
            "No-TSO TARA granted while the second regulator still limited GALT");
    auto requesterCurrentTimeAfterFirstRegulator =
        requesterTime.factory->makeInitial();
    require(requesterCurrentTimeAfterFirstRegulator != nullptr,
            "No-TSO TARA could not allocate its first-regulator time query");
    requester.rtiAmbassador().queryLogicalTime(
        *requesterCurrentTimeAfterFirstRegulator);
    requireTimeEquals(*requesterCurrentTimeAfterFirstRegulator,
                      *requesterTime.initial,
                      "No-TSO TARA time after only one regulator advanced");
    auto const galtWhileSecondRegulatorIsBehind =
        requesterTime.factory->makeInitial();
    require(galtWhileSecondRegulatorIsBehind != nullptr &&
                requester.rtiAmbassador().queryGALT(
                    *galtWhileSecondRegulatorIsBehind),
            "No-TSO TARA lost GALT while the second regulator remained active");
    requireTimeEquals(*galtWhileSecondRegulatorIsBehind,
                      *initialGalt,
                      "No-TSO TARA GALT after only the primary regulator advanced");

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
            options, "No-TSO TARA second regulator advance");
    require(secondRegulator.recorder().timeAdvanceGrants().front().encoded ==
                encodeTime(*secondRegulatorTarget),
            "No-TSO TARA second regulator did not reach its requested time");
    auto const galtAfterBothRegulatorsAdvance =
        requesterTime.factory->makeInitial();
    require(galtAfterBothRegulatorsAdvance != nullptr &&
                requester.rtiAmbassador().queryGALT(
                    *galtAfterBothRegulatorsAdvance),
            "No-TSO TARA lost GALT after both regulators advanced");
    require(*galtAfterBothRegulatorsAdvance > *requestTime,
            "No-TSO TARA GALT did not clear the request after both regulators advanced");
  }

  waitFor(requester,
          [&] { return requester.recorder().timeAdvanceGrants().size() == 1U; },
          options, "No-TSO TARA grant at the requested time");
  require(requester.recorder().timedInteractions().empty(),
          "No-TSO TARA delivered a timestamped interaction");
  auto const grants = requester.recorder().timeAdvanceGrants();
  require(grants.size() == 1U && grants.front().encoded == encodeTime(*requestTime),
          "No-TSO TARA did not grant at its request limit");
  require(requester.recorder().callbackOrder() == std::vector<std::string>{"grant"},
          "No-TSO TARA produced an unexpected callback order");

  auto requesterCurrentTime = requesterTime.factory->makeInitial();
  require(requesterCurrentTime != nullptr,
          "No-TSO TARA could not allocate its final logical-time query value");
  requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
  requireTimeEquals(*requesterCurrentTime, *requestTime,
                    "No-TSO TARA final requester time");
  require(regulator.recorder().timeAdvanceGrants().size() ==
              (expectedNrgEnabled ? 0U : 1U),
          "No-TSO TARA used an unexpected number of primary regulator advances");
  require(secondRegulator.recorder().timeAdvanceGrants().size() ==
              (expectedNrgEnabled ? 0U : 1U),
          "No-TSO TARA used an unexpected number of second regulator advances");
  if (expectedNrgEnabled) {
    auto finalGalt = requesterTime.factory->makeInitial();
    require(finalGalt != nullptr && !requester.rtiAmbassador().queryGALT(*finalGalt),
            "No-TSO TARA NRG-enabled grant unexpectedly defined GALT");
  }

  requester.rtiAmbassador().disableTimeConstrained();
  requester.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  secondRegulator.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
}

void runNoTsoUndefinedGaltTaraMatrix(
    Options const& options,
    rti::CallbackModel model) {
  auto const disabledFom = options.scenarioFoms.find("tara-undefined-galt-nrg-disabled");
  auto const enabledFom = options.scenarioFoms.find("tara-undefined-galt-nrg-enabled");
  require(disabledFom != options.scenarioFoms.end() && !disabledFom->second.empty() &&
              enabledFom != options.scenarioFoms.end() && !enabledFom->second.empty(),
          "No-TSO TARA matrix requires NRG-disabled and NRG-enabled adapter FOMs");
  auto disabledOptions = options;
  disabledOptions.fom = disabledFom->second;
  runNoTsoUndefinedGaltTara(disabledOptions, model, false);
  auto enabledOptions = options;
  enabledOptions.fom = enabledFom->second;
  runNoTsoUndefinedGaltTara(enabledOptions, model, true);
}

void requireNoTaraProcessGrant(
    Session& requester,
    Options const& options,
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
}

void runNoTsoTaraRequesterProcess(
    Options const& options,
    rti::CallbackModel model,
    bool expectedNrgEnabled,
    std::filesystem::path const& directory) {
  auto const switchName = expectedNrgEnabled ? "enabled" : "disabled";
  auto const fom = options.fom;
  Session requester(options, model, "owner");
  requester.connect();
  auto const federation = federationName(
      options,
      std::string("tara-no-tso-undefined-galt-") + switchName);
  requester.rtiAmbassador().createFederationExecution(
      federation, fom.wstring(), options.logicalTimeImplementationName);
  writeNmrProcessMarker(directory, "federation-created.marker");
  requester.join(options.ownerFederateName, options.federateType, federation);

  auto requesterTime = makeTimeContext(requester);
  require(
      requester.rtiAmbassador().getNonRegulatedGrantSwitch() ==
          expectedNrgEnabled,
      "process-separated TARA requester has the wrong NRG switch");
  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "process-separated TARA requester Time Constrained callback");
  waitForNmrProcessMarker(directory, "regulator-one-ready.marker", options);
  waitForNmrProcessMarker(directory, "regulator-two-ready.marker", options);

  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  auto observedGalt = requesterTime.factory->makeInitial();
  require(
      observedGalt != nullptr &&
          !requester.rtiAmbassador().queryGALT(*observedGalt),
      "process-separated no-TSO TARA started with a defined GALT");
  requester.rtiAmbassador().timeAdvanceRequestAvailable(*requestTime);

  if (!expectedNrgEnabled) {
    requireNoTaraProcessGrant(
        requester,
        options,
        "process-separated TARA granted with NRG disabled and GALT undefined");
    auto currentTime = requesterTime.factory->makeInitial();
    require(currentTime != nullptr, "process-separated TARA lacks a time query value");
    requester.rtiAmbassador().queryLogicalTime(*currentTime);
    requireTimeEquals(
        *currentTime,
        *requesterTime.initial,
        "process-separated TARA advanced before either regulator resumed");
    writeNmrProcessMarker(directory, "requester-pending.marker");
    waitForNmrProcessMarker(directory, "regulator-one-resumed.marker", options);
    waitForNmrProcessMarker(directory, "regulator-two-resumed.marker", options);
    writeNmrProcessMarker(directory, "advance-primary.marker");
    waitForNmrProcessMarker(directory, "regulator-one-advanced.marker", options);

    requireNoTaraProcessGrant(
        requester,
        options,
        "process-separated TARA granted while the second regulator remained behind");
    requester.rtiAmbassador().queryLogicalTime(*currentTime);
    requireTimeEquals(
        *currentTime,
        *requesterTime.initial,
        "process-separated TARA advanced while one regulator remained behind");
    require(
        requester.rtiAmbassador().queryGALT(*observedGalt) &&
            *observedGalt < *requestTime,
        "process-separated TARA lost its below-request bound after one regulator advanced");
    writeNmrProcessMarker(directory, "advance-second.marker");
    waitForNmrProcessMarker(directory, "regulator-two-advanced.marker", options);
  }

  waitFor(
      requester,
      [&] { return requester.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "process-separated TARA grant at the requested time");
  auto const grants = requester.recorder().timeAdvanceGrants();
  require(
      grants.front().encoded == encodeTime(*requestTime) &&
          requester.recorder().timedInteractions().empty() &&
          requester.recorder().callbackOrder() == std::vector<std::string>{"grant"},
      "process-separated no-TSO TARA did not produce one request-time grant");
  auto currentTime = requesterTime.factory->makeInitial();
  require(currentTime != nullptr, "process-separated TARA lacks a final time query value");
  requester.rtiAmbassador().queryLogicalTime(*currentTime);
  requireTimeEquals(*currentTime, *requestTime, "process-separated TARA final time");
  if (expectedNrgEnabled) {
    require(
        !requester.rtiAmbassador().queryGALT(*observedGalt),
        "process-separated NRG-enabled TARA unexpectedly defined GALT");
  } else {
    require(
        requester.rtiAmbassador().queryGALT(*observedGalt) &&
            *observedGalt > *requestTime,
        "process-separated TARA granted before both regulators cleared the request");
  }
  writeNmrProcessMarker(directory, "requester-completed.marker");
  waitForNmrProcessMarker(directory, "regulator-one-resigned.marker", options);
  waitForNmrProcessMarker(directory, "regulator-two-resigned.marker", options);
  requester.rtiAmbassador().disableTimeConstrained();
  requester.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
}

void runNoTsoTaraRegulatorProcess(
    Options const& options,
    rti::CallbackModel model,
    bool expectedNrgEnabled,
    bool secondRegulator,
    std::filesystem::path const& directory) {
  auto const switchName = expectedNrgEnabled ? "enabled" : "disabled";
  waitForNmrProcessMarker(directory, "federation-created.marker", options);
  Session regulator(options, model, "member");
  regulator.connect();
  auto const federation = federationName(
      options,
      std::string("tara-no-tso-undefined-galt-") + switchName);
  regulator.join(options.memberFederateName, options.federateType, federation);
  auto regulatorTime = makeTimeContext(regulator);
  require(
      regulator.rtiAmbassador().getNonRegulatedGrantSwitch() ==
          expectedNrgEnabled,
      "process-separated TARA regulator has the wrong NRG switch");

  auto const initialLookaheadUnits = secondRegulator ? 1U : 2U;
  auto const initialGalt = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      initialLookaheadUnits);
  auto lookahead = regulatorTime.factory->makeZero();
  require(lookahead != nullptr, "process-separated TARA regulator lacks lookahead");
  lookahead->setToDifference(*initialGalt, *regulatorTime.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "process-separated TARA regulator Time Regulation callback");
  regulator.rtiAmbassador().disableTimeRegulation();
  auto const disablePumpDeadline = Clock::now() + std::chrono::milliseconds(50);
  while (Clock::now() < disablePumpDeadline) {
    regulator.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  auto const readyMarker = secondRegulator
      ? "regulator-two-ready.marker"
      : "regulator-one-ready.marker";
  writeNmrProcessMarker(directory, readyMarker);

  if (!expectedNrgEnabled) {
    waitForNmrProcessMarker(directory, "requester-pending.marker", options);
    regulator.rtiAmbassador().enableTimeRegulation(*lookahead);
    waitFor(
        regulator,
        [&] { return regulator.recorder().timeRegulationEnabled().size() == 2U; },
        options,
        "process-separated TARA regulator resumed Time Regulation callback");
    auto const resumedMarker = secondRegulator
        ? "regulator-two-resumed.marker"
        : "regulator-one-resumed.marker";
    writeNmrProcessMarker(directory, resumedMarker);
    waitForNmrProcessMarker(
        directory,
        secondRegulator ? "advance-second.marker" : "advance-primary.marker",
        options);
    auto const target = timeAfter(
        *regulatorTime.factory,
        *regulatorTime.initial,
        *regulatorTime.epsilon,
        11U);
    regulator.rtiAmbassador().timeAdvanceRequest(*target);
    waitFor(
        regulator,
        [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
        options,
        "process-separated TARA regulator advance");
    require(
        regulator.recorder().timeAdvanceGrants().front().encoded == encodeTime(*target),
        "process-separated TARA regulator reached the wrong time");
    auto const advancedMarker = secondRegulator
        ? "regulator-two-advanced.marker"
        : "regulator-one-advanced.marker";
    writeNmrProcessMarker(directory, advancedMarker);
  }

  waitForNmrProcessMarker(directory, "requester-completed.marker", options);
  require(
      regulator.recorder().timeAdvanceGrants().size() ==
          (expectedNrgEnabled ? 0U : 1U),
      "process-separated TARA regulator made an unexpected number of advances");
  regulator.resign(rti::NO_ACTION);
  regulator.disconnect();
  auto const resignedMarker = secondRegulator
      ? "regulator-two-resigned.marker"
      : "regulator-one-resigned.marker";
  writeNmrProcessMarker(directory, resignedMarker);
}

void runNoTsoUndefinedGaltTaraProcesses(
    Options const& options,
    rti::CallbackModel model) {
  auto const disabledFom = options.scenarioFoms.find("tara-undefined-galt-nrg-disabled");
  auto const enabledFom = options.scenarioFoms.find("tara-undefined-galt-nrg-enabled");
  require(
      disabledFom != options.scenarioFoms.end() && !disabledFom->second.empty() &&
          enabledFom != options.scenarioFoms.end() && !enabledFom->second.empty(),
      "process-separated no-TSO TARA requires both NRG adapter FOMs");
  auto const runDirectory = nextNmrProcessRunDirectory(options);
  for (bool const expectedNrgEnabled : {false, true}) {
    auto variantOptions = options;
    auto const switchName = expectedNrgEnabled ? "enabled" : "disabled";
    variantOptions.fom = expectedNrgEnabled ? enabledFom->second : disabledFom->second;
    auto const directory = runDirectory / switchName;
    std::filesystem::create_directories(directory);
    if (options.nmrProcessRole == "requester") {
      runNoTsoTaraRequesterProcess(
          variantOptions, model, expectedNrgEnabled, directory);
    } else if (options.nmrProcessRole == "publisher") {
      runNoTsoTaraRegulatorProcess(
          variantOptions, model, expectedNrgEnabled, false, directory);
    } else if (options.nmrProcessRole == "second-publisher") {
      runNoTsoTaraRegulatorProcess(
          variantOptions, model, expectedNrgEnabled, true, directory);
    } else {
      throw std::runtime_error(
          "TARA process role must be requester, publisher, or second-publisher");
    }
  }
}

}  // namespace

void scenarioTaraUndefinedGaltNrg(
    Options const& options,
    rti::CallbackModel model) {
  runUndefinedGaltTaraMatrix(options, model);
}

void scenarioTaraUndefinedGaltNrgContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTaraUndefinedGaltNrg(options, model);
}

void scenarioTaraNoTsoUndefinedGaltNrg(
    Options const& options,
    rti::CallbackModel model) {
  if (!options.nmrProcessRole.empty()) {
    runNoTsoUndefinedGaltTaraProcesses(options, model);
    return;
  }
  runNoTsoUndefinedGaltTaraMatrix(options, model);
}

void scenarioTaraNoTsoUndefinedGaltNrgContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTaraNoTsoUndefinedGaltNrg(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
