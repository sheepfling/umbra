#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimeRegulationEnableActualLookaheadTsoSend(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Time Regulation lookahead requires an adapter-supplied logical-time implementation");

  Session sender(options, model, "owner");
  auto const federation = federationName(
      options, "time-regulation-enable-actual-lookahead-tso-send");
  sender.connect();
  sender.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  sender.join(options.ownerFederateName, options.federateType, federation);

  auto const interaction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(interaction.isValid(), "Time Regulation lookahead interaction lookup failed");
  sender.rtiAmbassador().publishInteractionClass(interaction);
  sender.rtiAmbassador().changeInteractionOrderType(interaction, rti::TIMESTAMP);

  auto const time = makeTimeContext(sender);
  auto const one = timeAfter(
      *time.factory, *time.initial, *time.epsilon, 1U);
  auto const two = timeAfter(
      *time.factory, *time.initial, *time.epsilon, 2U);
  auto requestedLookahead = time.factory->makeZero();
  require(requestedLookahead != nullptr, "Could not allocate requested lookahead");
  requestedLookahead->setToDifference(*two, *time.initial);

  sender.rtiAmbassador().enableTimeRegulation(*requestedLookahead);
  waitFor(
      sender,
      [&] { return sender.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Time Regulation Enabled callback for actual-lookahead send boundary");
  require(
      sender.recorder().timeRegulationEnabled().front().encoded ==
          encodeTime(*time.initial),
      "Time Regulation Enabled reported the wrong assigned logical time");

  auto actualLookahead = time.factory->makeZero();
  auto currentTime = time.factory->makeInitial();
  require(
      actualLookahead != nullptr && currentTime != nullptr,
      "Could not allocate post-enable time queries");
  sender.rtiAmbassador().queryLookahead(*actualLookahead);
  sender.rtiAmbassador().queryLogicalTime(*currentTime);
  require(
      *actualLookahead == *requestedLookahead && *currentTime == *time.initial,
      "Enable Time Regulation did not retain the requested lookahead and assigned time");

  rti::ParameterHandleValueMap const parameters;
  rti::VariableLengthData const tag;
  bool belowFloorRejected = false;
  try {
    static_cast<void>(sender.rtiAmbassador().sendInteraction(
        interaction, parameters, tag, *one));
  } catch (rti::InvalidLogicalTime const&) {
    belowFloorRejected = true;
  }
  require(
      belowFloorRejected,
      "Send Interaction accepted a TSO timestamp below the actual-lookahead floor");
  auto const retraction = sender.rtiAmbassador().sendInteraction(
      interaction, parameters, tag, *two);
  require(
      retraction.isValid(),
      "Send Interaction rejected a TSO timestamp at the actual-lookahead floor");

  sender.rtiAmbassador().disableTimeRegulation();
  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  sender.disconnect();
}

void scenarioTimeRegulationEnableActualLookaheadTsoSendContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeRegulationEnableActualLookaheadTsoSend(options, model);
}

void scenarioZeroLookaheadTarTsoEqualityRejection(
    Options const& options,
    rti::CallbackModel model) {
  Session sender(options, model, "zero-lookahead-tar-tso-sender");
  auto const federation = federationName(
      options, "zero-lookahead-tar-tso-equality-rejection");
  sender.connect();
  sender.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  sender.join(options.ownerFederateName, options.federateType, federation);

  auto const interaction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(interaction.isValid(),
          "Zero-lookahead TAR interaction lookup failed");
  sender.rtiAmbassador().publishInteractionClass(interaction);
  sender.rtiAmbassador().changeInteractionOrderType(
      interaction, rti::TIMESTAMP);

  auto const time = makeTimeContext(sender);
  auto zeroLookahead = time.factory->makeZero();
  auto const grantTime = timeAfter(
      *time.factory, *time.initial, *time.epsilon, 3U);
  auto const laterTime = timeAfter(
      *time.factory, *grantTime, *time.epsilon, 1U);
  require(zeroLookahead != nullptr,
          "Zero-lookahead TAR could not allocate its interval");
  require(zeroLookahead->isZero(),
          "Logical-time factory returned a nonzero zero-lookahead interval");

  sender.rtiAmbassador().enableTimeRegulation(*zeroLookahead);
  waitFor(
      sender,
      [&] { return sender.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "zero-lookahead TAR Time Regulation Enabled callback");
  require(
      sender.recorder().timeRegulationEnabled().front().encoded ==
          encodeTime(*time.initial),
      "Zero-lookahead TAR regulation began at the wrong logical time");
  auto actualLookahead = time.factory->makeZero();
  require(actualLookahead != nullptr,
          "Zero-lookahead TAR could not allocate its lookahead query");
  sender.rtiAmbassador().queryLookahead(*actualLookahead);
  require(*actualLookahead == *zeroLookahead,
          "Time Regulation did not retain zero lookahead");

  sender.rtiAmbassador().timeAdvanceRequest(*grantTime);
  waitFor(
      sender,
      [&] { return sender.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "zero-lookahead TAR grant");
  auto const grants = sender.recorder().timeAdvanceGrants();
  require(
      grants.size() == 1U && grants.front().encoded == encodeTime(*grantTime),
      "Zero-lookahead TAR granted the wrong logical time");

  auto actualTime = time.factory->makeInitial();
  require(actualTime != nullptr,
          "Zero-lookahead TAR could not allocate its logical-time query");
  sender.rtiAmbassador().queryLogicalTime(*actualTime);
  require(*actualTime == *grantTime,
          "Zero-lookahead TAR did not advance to its granted logical time");

  rti::ParameterHandleValueMap const parameters;
  rti::VariableLengthData const tag;
  bool atGrantTimeRejected = false;
  try {
    static_cast<void>(sender.rtiAmbassador().sendInteraction(
        interaction, parameters, tag, *grantTime));
  } catch (rti::InvalidLogicalTime const&) {
    atGrantTimeRejected = true;
  }
  require(
      atGrantTimeRejected,
      "Zero-lookahead TAR allowed a TSO send at the granted logical time");

  auto const laterRetraction = sender.rtiAmbassador().sendInteraction(
      interaction, parameters, tag, *laterTime);
  require(
      laterRetraction.isValid(),
      "Zero-lookahead TAR rejected a TSO timestamp above the granted time");

  sender.rtiAmbassador().disableTimeRegulation();
  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  sender.disconnect();
}

void scenarioZeroLookaheadTarTsoEqualityRejectionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioZeroLookaheadTarTsoEqualityRejection(options, model);
}

void scenarioZeroLookaheadNmrTsoEqualityRejection(
    Options const& options,
    rti::CallbackModel model) {
  Session sender(options, model, "zero-lookahead-nmr-tso-sender");
  auto const federation = federationName(
      options, "zero-lookahead-nmr-tso-equality-rejection");
  sender.connect();
  sender.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  sender.join(options.ownerFederateName, options.federateType, federation);

  auto const interaction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(interaction.isValid(),
          "Zero-lookahead NMR interaction lookup failed");
  sender.rtiAmbassador().publishInteractionClass(interaction);
  sender.rtiAmbassador().changeInteractionOrderType(
      interaction, rti::TIMESTAMP);

  auto const time = makeTimeContext(sender);
  auto zeroLookahead = time.factory->makeZero();
  auto const grantTime = timeAfter(
      *time.factory, *time.initial, *time.epsilon, 3U);
  auto const laterTime = timeAfter(
      *time.factory, *grantTime, *time.epsilon, 1U);
  require(zeroLookahead != nullptr,
          "Zero-lookahead NMR could not allocate its interval");
  require(zeroLookahead->isZero(),
          "Logical-time factory returned a nonzero zero-lookahead interval");

  sender.rtiAmbassador().enableTimeRegulation(*zeroLookahead);
  waitFor(
      sender,
      [&] { return sender.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "zero-lookahead NMR Time Regulation Enabled callback");
  sender.rtiAmbassador().nextMessageRequest(*grantTime);
  waitFor(
      sender,
      [&] { return sender.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "zero-lookahead NMR grant");
  auto const grants = sender.recorder().timeAdvanceGrants();
  require(
      grants.size() == 1U && grants.front().encoded == encodeTime(*grantTime),
      "Zero-lookahead NMR granted the wrong logical time");
  auto actualTime = time.factory->makeInitial();
  require(actualTime != nullptr,
          "Zero-lookahead NMR could not allocate its logical-time query");
  sender.rtiAmbassador().queryLogicalTime(*actualTime);
  require(*actualTime == *grantTime,
          "Zero-lookahead NMR did not advance to its granted logical time");

  rti::ParameterHandleValueMap const parameters;
  rti::VariableLengthData const tag;
  bool atGrantTimeRejected = false;
  try {
    static_cast<void>(sender.rtiAmbassador().sendInteraction(
        interaction, parameters, tag, *grantTime));
  } catch (rti::InvalidLogicalTime const&) {
    atGrantTimeRejected = true;
  }
  require(
      atGrantTimeRejected,
      "Zero-lookahead NMR allowed a TSO send at the granted logical time");

  auto const laterRetraction = sender.rtiAmbassador().sendInteraction(
      interaction, parameters, tag, *laterTime);
  require(
      laterRetraction.isValid(),
      "Zero-lookahead NMR rejected a TSO timestamp above the granted time");

  sender.rtiAmbassador().disableTimeRegulation();
  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  sender.disconnect();
}

void scenarioZeroLookaheadNmrTsoEqualityRejectionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioZeroLookaheadNmrTsoEqualityRejection(options, model);
}

void scenarioZeroLookaheadNmrContinuationTaraEqualityAcceptance(
    Options const& options,
    rti::CallbackModel model) {
  Session sender(options, model, "zero-lookahead-nmr-continuation-sender");
  auto const federation = federationName(
      options, "zero-lookahead-nmr-continuation-tara-equality-acceptance");
  sender.connect();
  sender.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  sender.join(options.ownerFederateName, options.federateType, federation);

  auto const interaction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(interaction.isValid(),
          "Zero-lookahead NMR continuation interaction lookup failed");
  sender.rtiAmbassador().publishInteractionClass(interaction);
  sender.rtiAmbassador().changeInteractionOrderType(
      interaction, rti::TIMESTAMP);

  auto const time = makeTimeContext(sender);
  auto zeroLookahead = time.factory->makeZero();
  auto const nmrGrantTime = timeAfter(
      *time.factory, *time.initial, *time.epsilon, 3U);
  auto const taraGrantTime = timeAfter(
      *time.factory, *nmrGrantTime, *time.epsilon, 1U);
  require(zeroLookahead != nullptr,
          "Zero-lookahead NMR continuation could not allocate its interval");
  require(zeroLookahead->isZero(),
          "Logical-time factory returned nonzero continuation lookahead");

  sender.rtiAmbassador().enableTimeRegulation(*zeroLookahead);
  waitFor(
      sender,
      [&] { return sender.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "zero-lookahead NMR continuation Time Regulation Enabled callback");
  sender.rtiAmbassador().nextMessageRequest(*nmrGrantTime);
  waitFor(
      sender,
      [&] { return sender.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "zero-lookahead NMR continuation first grant");
  auto const firstGrant = sender.recorder().timeAdvanceGrants();
  require(
      firstGrant.front().encoded == encodeTime(*nmrGrantTime),
      "Zero-lookahead NMR continuation received the wrong first grant");

  rti::ParameterHandleValueMap const parameters;
  rti::VariableLengthData const tag;
  bool atNmrGrantRejected = false;
  try {
    static_cast<void>(sender.rtiAmbassador().sendInteraction(
        interaction, parameters, tag, *nmrGrantTime));
  } catch (rti::InvalidLogicalTime const&) {
    atNmrGrantRejected = true;
  }
  require(
      atNmrGrantRejected,
      "Zero-lookahead NMR continuation allowed equality before a later advance");

  sender.rtiAmbassador().timeAdvanceRequestAvailable(*taraGrantTime);
  waitFor(
      sender,
      [&] { return sender.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "zero-lookahead NMR continuation TARA grant");
  auto const grants = sender.recorder().timeAdvanceGrants();
  require(
      grants.size() == 2U &&
          grants.back().encoded == encodeTime(*taraGrantTime),
      "Zero-lookahead NMR continuation TARA granted the wrong logical time");
  auto actualTime = time.factory->makeInitial();
  require(actualTime != nullptr,
          "Zero-lookahead NMR continuation could not allocate its time query");
  sender.rtiAmbassador().queryLogicalTime(*actualTime);
  require(*actualTime == *taraGrantTime,
          "Zero-lookahead NMR continuation did not reach the TARA grant time");

  auto const retraction = sender.rtiAmbassador().sendInteraction(
      interaction, parameters, tag, *taraGrantTime);
  require(
      retraction.isValid(),
      "A later TARA advance did not lift the extra NMR equality restriction");

  sender.rtiAmbassador().disableTimeRegulation();
  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  sender.disconnect();
}

void scenarioZeroLookaheadNmrContinuationTaraEqualityAcceptanceContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioZeroLookaheadNmrContinuationTaraEqualityAcceptance(options, model);
}

void scenarioZeroLookaheadNmraTsoEqualityAcceptance(
    Options const& options,
    rti::CallbackModel model) {
  Session sender(options, model, "zero-lookahead-nmra-tso-sender");
  auto const federation = federationName(
      options, "zero-lookahead-nmra-tso-equality-acceptance");
  sender.connect();
  sender.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  sender.join(options.ownerFederateName, options.federateType, federation);

  auto const interaction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(interaction.isValid(),
          "Zero-lookahead NMRA interaction lookup failed");
  sender.rtiAmbassador().publishInteractionClass(interaction);
  sender.rtiAmbassador().changeInteractionOrderType(
      interaction, rti::TIMESTAMP);

  auto const time = makeTimeContext(sender);
  auto zeroLookahead = time.factory->makeZero();
  auto const grantTime = timeAfter(
      *time.factory, *time.initial, *time.epsilon, 3U);
  require(zeroLookahead != nullptr,
          "Zero-lookahead NMRA could not allocate its interval");
  require(zeroLookahead->isZero(),
          "Logical-time factory returned a nonzero zero-lookahead interval");

  sender.rtiAmbassador().enableTimeRegulation(*zeroLookahead);
  waitFor(
      sender,
      [&] { return sender.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "zero-lookahead NMRA Time Regulation Enabled callback");
  sender.rtiAmbassador().nextMessageRequestAvailable(*grantTime);
  waitFor(
      sender,
      [&] { return sender.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "zero-lookahead NMRA grant");
  auto const grants = sender.recorder().timeAdvanceGrants();
  require(
      grants.size() == 1U && grants.front().encoded == encodeTime(*grantTime),
      "Zero-lookahead NMRA granted the wrong logical time");

  rti::ParameterHandleValueMap const parameters;
  rti::VariableLengthData const tag;
  auto const retraction = sender.rtiAmbassador().sendInteraction(
      interaction, parameters, tag, *grantTime);
  require(
      retraction.isValid(),
      "Zero-lookahead NMRA rejected a TSO timestamp equal to its grant time");

  sender.rtiAmbassador().disableTimeRegulation();
  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  sender.disconnect();
}

void scenarioZeroLookaheadNmraTsoEqualityAcceptanceContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioZeroLookaheadNmraTsoEqualityAcceptance(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
