#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioTimestampedDirectedZeroLookaheadTarSendBoundary(
    Options const& options,
    rti::CallbackModel model) {
  require(
      model == rti::HLA_EVOKED,
      "directed zero-lookahead TAR send boundary requires HLA_EVOKED callbacks");
  require(
      !options.logicalTimeImplementationName.empty(),
      "directed zero-lookahead TAR send boundary requires adapter-supplied logical time");

  Session publisher(options, model, "member");
  Session owner(options, model, "owner");
  auto const federation = federationName(
      options, "timestamped-directed-zero-lookahead-tar-send-boundary");
  publisher.connect();
  owner.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.memberFederateName + L"-directed-zero-tar-sender",
      options.federateType,
      federation);
  owner.join(
      options.ownerFederateName + L"-directed-zero-tar-owner",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.attributeName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const ownerInteraction = owner.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction, options.parameterName);
  auto const ownerParameter = owner.rtiAmbassador().getParameterHandle(
      ownerInteraction, options.parameterName);
  require(
      publisherClass.isValid() && ownerClass.isValid() &&
          publisherAttribute.isValid() && ownerAttribute.isValid() &&
          publisherInteraction.isValid() && ownerInteraction.isValid() &&
          publisherParameter.isValid() && ownerParameter.isValid(),
      "directed zero-lookahead TAR adapter FOM returned invalid handles");

  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass, rti::AttributeHandleSet{ownerAttribute});
  publisher.rtiAmbassador().subscribeObjectClassAttributes(
      publisherClass, rti::AttributeHandleSet{publisherAttribute}, true, L"");
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherClass, rti::InteractionClassHandleSet{publisherInteraction});
  owner.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      ownerClass, rti::InteractionClassHandleSet{ownerInteraction}, false);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction, rti::TIMESTAMP);
  auto const target = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(target.isValid(), "directed zero-lookahead TAR target was invalid");
  waitFor(
      publisher,
      [&] { return publisher.recorder().hasDiscovery(target); },
      options,
      "directed zero-lookahead TAR sender target discovery");

  auto const publisherTime = makeTimeContext(publisher);
  auto const ownerTime = makeTimeContext(owner);
  require(
      publisherTime.factory->getName() == ownerTime.factory->getName() &&
          encodeTime(*publisherTime.initial) == encodeTime(*ownerTime.initial),
      "directed zero-lookahead TAR members selected inconsistent time contexts");
  owner.rtiAmbassador().enableTimeConstrained();
  waitFor(
      owner,
      [&] { return owner.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "directed zero-lookahead TAR owner constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.zero);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "directed zero-lookahead TAR publisher regulation callback");

  auto const requestedTime = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 5U);
  auto const futureTime = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 6U);
  std::vector<std::uint8_t> const payloadBytes{0xD7U, 0x05U};
  std::vector<std::uint8_t> const tagBytes{0xD7U, 0x06U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(payloadBytes.data(), payloadBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());

  publisher.rtiAmbassador().timeAdvanceRequest(*requestedTime);
  requireException(
      [&] {
        publisher.rtiAmbassador().sendDirectedInteraction(
            publisherInteraction, target, parameters, tag, *requestedTime);
      },
      L"InvalidLogicalTime",
      "directed zero-lookahead TAR send at requested time");
  auto const futureRetraction = publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction, target, parameters, tag, *futureTime);
  require(
      futureRetraction.isValid(),
      "directed zero-lookahead TAR rejected a send strictly after requested time");
  require(
      publisher.recorder().timeAdvanceGrants().empty(),
      "directed zero-lookahead TAR grant was serviced before explicit evocation");
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "directed zero-lookahead TAR publisher grant");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*requestedTime) &&
          owner.recorder().timedDirectedInteractions().empty(),
      "directed zero-lookahead TAR grant or pre-advance delivery was incorrect");

  owner.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  publisher.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedDirectedZeroLookaheadSendAtGrant(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "directed zero-lookahead post-grant send requires adapter-supplied logical time");

  Session publisher(options, model, "member");
  Session owner(options, model, "owner");
  auto const federation = federationName(
      options, "timestamped-directed-zero-lookahead-send-at-grant");
  publisher.connect();
  owner.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  publisher.join(
      options.memberFederateName + L"-directed-zero-grant-sender",
      options.federateType,
      federation);
  owner.join(
      options.ownerFederateName + L"-directed-zero-grant-owner",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.attributeName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const ownerInteraction = owner.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction, options.parameterName);
  auto const ownerParameter = owner.rtiAmbassador().getParameterHandle(
      ownerInteraction, options.parameterName);
  require(
      publisherClass.isValid() && ownerClass.isValid() &&
          publisherAttribute.isValid() && ownerAttribute.isValid() &&
          publisherInteraction.isValid() && ownerInteraction.isValid() &&
          publisherParameter.isValid() && ownerParameter.isValid() &&
          publisherInteraction == ownerInteraction &&
          publisherParameter == ownerParameter,
      "directed zero-lookahead post-grant FOM returned inconsistent handles");

  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass, rti::AttributeHandleSet{ownerAttribute});
  publisher.rtiAmbassador().subscribeObjectClassAttributes(
      publisherClass, rti::AttributeHandleSet{publisherAttribute}, true, L"");
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherClass, rti::InteractionClassHandleSet{publisherInteraction});
  owner.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      ownerClass, rti::InteractionClassHandleSet{ownerInteraction}, false);
  publisher.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      publisherClass, rti::InteractionClassHandleSet{publisherInteraction}, false);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction, rti::TIMESTAMP);
  auto const target = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(target.isValid(), "directed zero-lookahead post-grant target was invalid");
  waitFor(
      publisher,
      [&] { return publisher.recorder().hasDiscovery(target); },
      options,
      "directed zero-lookahead post-grant target discovery");

  auto const publisherTime = makeTimeContext(publisher);
  auto const ownerTime = makeTimeContext(owner);
  require(
      publisherTime.factory->getName() == ownerTime.factory->getName() &&
          encodeTime(*publisherTime.initial) == encodeTime(*ownerTime.initial),
      "directed zero-lookahead post-grant members selected inconsistent time contexts");
  owner.rtiAmbassador().enableTimeConstrained();
  waitFor(
      owner,
      [&] { return owner.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "directed zero-lookahead post-grant owner constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.zero);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "directed zero-lookahead post-grant publisher regulation callback");
  auto actualLookahead = publisherTime.factory->makeZero();
  require(actualLookahead != nullptr, "could not allocate Query Lookahead output");
  publisher.rtiAmbassador().queryLookahead(*actualLookahead);
  require(
      *actualLookahead == *publisherTime.zero,
      "directed zero-lookahead post-grant sender did not retain zero lookahead");

  auto const grantedTime = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 5U);
  auto const nextTime = timeAfter(
      *publisherTime.factory, *grantedTime, *publisherTime.epsilon, 1U);
  publisher.rtiAmbassador().timeAdvanceRequest(*grantedTime);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "directed zero-lookahead sender initial grant");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*grantedTime),
      "directed zero-lookahead sender received wrong initial grant");
  auto queriedTime = publisherTime.factory->makeInitial();
  require(queriedTime != nullptr, "could not allocate Query Logical Time output");
  publisher.rtiAmbassador().queryLogicalTime(*queriedTime);
  require(
      *queriedTime == *grantedTime,
      *queriedTime < *grantedTime
          ? "directed zero-lookahead sender logical time lagged its grant"
          : "directed zero-lookahead sender logical time exceeded its grant");
  publisher.rtiAmbassador().queryLookahead(*actualLookahead);
  require(
      *actualLookahead == *publisherTime.zero,
      "directed zero-lookahead sender lost zero lookahead after grant");

  std::vector<std::uint8_t> const payloadBytes{0xD7U, 0x15U};
  std::vector<std::uint8_t> const tagBytes{0xD7U, 0x16U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(payloadBytes.data(), payloadBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction, target, parameters, tag, *grantedTime);
  require(
      retraction.isValid(),
      "directed zero-lookahead send at just-granted time was rejected");
  auto const secondRetraction = publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction, target, parameters, tag, *nextTime);
  require(
      secondRetraction.isValid() &&
          copyBytes(secondRetraction.encode()) != copyBytes(retraction.encode()),
      "concurrent directed sends did not receive distinct federation retraction designators");
  publisher.rtiAmbassador().retract(secondRetraction);
  require(
      owner.recorder().timedDirectedInteractions().empty(),
      "directed zero-lookahead TSO arrived before receiver advanced");

  publisher.rtiAmbassador().timeAdvanceRequest(*nextTime);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "directed zero-lookahead sender forward grant");
  owner.rtiAmbassador().timeAdvanceRequest(*grantedTime);
  waitFor(
      owner,
      [&] { return owner.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "directed zero-lookahead receiver matching grant");
  auto const interactions = owner.recorder().timedDirectedInteractions();
  require(
      interactions.size() == 1U &&
          owner.recorder().callbackOrder() ==
              std::vector<std::string>{"directed", "grant"},
      "directed zero-lookahead TSO did not precede receiver grant");
  auto const& interaction = interactions.front();
  require(
      interaction.interaction == ownerInteraction &&
          interaction.object == target &&
          interaction.producer == publisher.federateHandle() &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(ownerParameter) == 1U &&
          interaction.time == encodeTime(*grantedTime) &&
          interaction.sentOrder == rti::TIMESTAMP &&
          interaction.receivedOrder == rti::TIMESTAMP &&
          interaction.tag == tagBytes && interaction.retractionPresent &&
          interaction.retraction == copyBytes(retraction.encode()) &&
          copyBytes(interaction.parameters.at(ownerParameter)) == payloadBytes,
      "directed zero-lookahead callback changed target, payload, or metadata");
  publisher.pump();
  require(
      publisher.recorder().directedInteractions().empty() &&
          publisher.recorder().timedDirectedInteractions().empty(),
      "directed zero-lookahead sender received its own directed interaction");
  require(
      owner.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*grantedTime),
      "directed zero-lookahead receiver received wrong grant time");

  owner.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
