#include "tck_restore_terminal_tso_directed_interaction.hpp"

#include <array>

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestoreTerminalTsoDirectedInteraction(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Terminal directed-interaction restore has native evidence only for HLA_EVOKED");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Terminal directed restore requires adapter-supplied FOM and time implementation");
  Session sender(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "restore-terminal-tso-directed");
  std::wstring const saveLabel = L"tck-terminal-directed-retraction-baseline";
  std::array<std::uint8_t, 4> const tagBytes{'T', 'D', '-', 'T'};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  sender.connect();
  receiver.connect();
  sender.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  sender.join(options.ownerFederateName, options.federateType, federation);
  receiver.join(L"tck-terminal-directed-receiver", options.federateType, federation);
  auto const senderClass = sender.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const senderAttribute = sender.rtiAmbassador().getAttributeHandle(
      senderClass, options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass, options.attributeName);
  auto const senderInteraction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(senderClass.isValid() && receiverClass.isValid() &&
              senderAttribute.isValid() && receiverAttribute.isValid() &&
              senderInteraction.isValid() && receiverInteraction.isValid(),
          "Adapter FOM lacks terminal directed object, attribute, or interaction class");
  sender.rtiAmbassador().publishObjectClassAttributes(
      senderClass, rti::AttributeHandleSet{senderAttribute});
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass, rti::AttributeHandleSet{receiverAttribute});
  sender.rtiAmbassador().publishObjectClassDirectedInteractions(
      senderClass, rti::InteractionClassHandleSet{senderInteraction});
  receiver.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      receiverClass, rti::InteractionClassHandleSet{receiverInteraction}, true);
  auto const target = sender.rtiAmbassador().registerObjectInstance(senderClass);
  require(target.isValid(), "Terminal directed-interaction target was invalid");
  waitForSessions({&receiver}, [&] {
    return receiver.recorder().hasDiscovery(target);
  }, options, "terminal directed target discovery");

  receiver.rtiAmbassador().enableTimeConstrained();
  sender.rtiAmbassador().changeInteractionOrderType(senderInteraction, rti::TIMESTAMP);
  auto const time = makeTimeContext(sender);
  auto const time1 = timeAfter(*time.factory, *time.initial, *time.epsilon, 1U);
  auto const time6 = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  auto lookahead = time.factory->makeZero();
  require(lookahead != nullptr, "Terminal directed lookahead factory returned null");
  for (unsigned step = 0U; step != 5U; ++step) *lookahead += *time.epsilon;
  sender.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitForSessions({&sender}, [&] {
    return sender.recorder().timeRegulationEnabled().size() == 1U;
  }, options, "terminal directed time-regulation enablement");
  auto const terminal = sender.rtiAmbassador().sendDirectedInteraction(
      senderInteraction, target, rti::ParameterHandleValueMap{}, tag, *time6);
  require(terminal.isValid() && receiver.recorder().timedDirectedInteractions().empty(),
          "Terminal directed interaction arrived before recipient grant");
  sender.rtiAmbassador().retract(terminal);
  receiver.pump();
  require(receiver.recorder().timedDirectedInteractions().empty(),
          "Terminal directed interaction was delivered after retraction");
  requireException([&] { sender.rtiAmbassador().retract(terminal); },
                   L"MessageCanNoLongerBeRetracted",
                   "terminal directed designator before save");

  auto const admission = sender.rtiAmbassador().sendDirectedInteraction(
      senderInteraction, target, rti::ParameterHandleValueMap{}, tag, *time6);
  require(admission.isValid(), "Directed admission passel lacked a retraction handle");
  receiver.rtiAmbassador().timeAdvanceRequest(*time1);
  sender.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions({&sender, &receiver}, [&] {
    return sender.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           receiver.federateSaveInitiations() == std::vector<std::wstring>{saveLabel};
  }, options, "terminal directed two-member save initiation");
  sender.rtiAmbassador().federateSaveBegun();
  receiver.rtiAmbassador().federateSaveBegun();
  sender.rtiAmbassador().federateSaveComplete();
  receiver.rtiAmbassador().federateSaveComplete();
  waitForSessions({&sender, &receiver}, [&] {
    return sender.federationSavedCount() == 1U &&
           receiver.federationSavedCount() == 1U;
  }, options, "terminal directed federation saved");

  sender.rtiAmbassador().retract(admission);
  receiver.pump();
  require(receiver.recorder().timedDirectedInteractions().empty(),
          "Post-save admission retraction delivered a directed callback");
  auto const discarded = sender.rtiAmbassador().sendDirectedInteraction(
      senderInteraction, target, rti::ParameterHandleValueMap{}, tag, *time6);
  require(discarded.isValid() &&
              copyBytes(terminal.encode()) != copyBytes(discarded.encode()),
          "Post-save directed interaction reused saved terminal designator");
  sender.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions({&sender, &receiver}, [&] {
    return sender.recorder().federationRestoreRequestsSucceeded().size() == 1U &&
           sender.federateRestoreInitiations().size() == 1U &&
           receiver.federateRestoreInitiations().size() == 1U &&
           sender.federateRestoreInitiations().front().label == saveLabel &&
           receiver.federateRestoreInitiations().front().label == saveLabel;
  }, options, "terminal directed restore initiation");
  sender.rtiAmbassador().federateRestoreComplete();
  receiver.rtiAmbassador().federateRestoreComplete();
  waitForSessions({&sender, &receiver}, [&] {
    return sender.federationRestoredCount() == 1U &&
           receiver.federationRestoredCount() == 1U;
  }, options, "terminal directed federation restored");
  requireException([&] { sender.rtiAmbassador().retract(terminal); },
                   L"MessageCanNoLongerBeRetracted",
                   "saved terminal directed classification");
  requireException([&] { sender.rtiAmbassador().retract(discarded); },
                   L"InvalidMessageRetractionHandle",
                   "discarded post-save directed designator");

  receiver.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      receiverClass, rti::InteractionClassHandleSet{receiverInteraction});
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass, rti::AttributeHandleSet{receiverAttribute});
  receiver.resign(rti::NO_ACTION);
  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  sender.disconnect();
}

}  // namespace

void scenarioRestoreTerminalTsoDirectedInteraction(
    Options const& options, rti::CallbackModel model) {
  runRestoreTerminalTsoDirectedInteraction(options, model);
}

void scenarioRestoreTerminalTsoDirectedInteractionContract(
    Options const& options, rti::CallbackModel model) {
  runRestoreTerminalTsoDirectedInteraction(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
