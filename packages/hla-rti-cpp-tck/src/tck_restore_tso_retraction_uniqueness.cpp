#include "tck_restore_tso_retraction_uniqueness.hpp"

#include <array>

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestoreTsoRetractionUniqueness(Options const& options,
                                       rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "TSO retraction-uniqueness restore has native evidence only for evoked callbacks");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Retraction-uniqueness restore requires adapter FOM and time implementation");
  Session sender(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "restore-tso-retraction-uniqueness");
  std::wstring const saveLabel = L"tck-tso-retraction-uniqueness";
  std::array<std::uint8_t, 4> const tagBytes{'U', 'N', 'I', 'Q'};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  sender.connect();
  receiver.connect();
  sender.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  sender.join(options.ownerFederateName, options.federateType + L"-publisher", federation);
  receiver.join(options.memberFederateName, options.federateType + L"-subscriber", federation);
  auto const senderClass = sender.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(options.objectClassName);
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
          "Adapter FOM lacks directed object, attribute, or interaction class");
  sender.rtiAmbassador().publishObjectClassAttributes(
      senderClass, rti::AttributeHandleSet{senderAttribute});
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass, rti::AttributeHandleSet{receiverAttribute});
  sender.rtiAmbassador().publishObjectClassDirectedInteractions(
      senderClass, rti::InteractionClassHandleSet{senderInteraction});
  sender.rtiAmbassador().changeInteractionOrderType(senderInteraction, rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      receiverClass, rti::InteractionClassHandleSet{receiverInteraction}, true);
  auto const target = sender.rtiAmbassador().registerObjectInstance(senderClass);
  require(target.isValid(), "Directed target registration returned an invalid handle");
  waitForSessions({&receiver}, [&] { return receiver.recorder().hasDiscovery(target); },
                  options, "directed target discovery");

  receiver.rtiAmbassador().enableTimeConstrained();
  receiver.pump();
  auto const time = makeTimeContext(sender);
  auto const time1 = timeAfter(*time.factory, *time.initial, *time.epsilon, 1U);
  auto const time6 = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  auto lookahead = time.factory->makeZero();
  require(lookahead != nullptr, "Time factory returned no lookahead interval");
  for (unsigned step = 0U; step != 5U; ++step) *lookahead += *time.epsilon;
  sender.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitForSessions({&sender}, [&] {
    return sender.recorder().timeRegulationEnabled().size() == 1U;
  }, options, "time-regulation enablement");

  auto const saved = sender.rtiAmbassador().sendDirectedInteraction(
      senderInteraction, target, rti::ParameterHandleValueMap{}, tag, *time6);
  require(saved.isValid() && receiver.recorder().timedDirectedInteractions().empty(),
          "Saved directed passel was not queued before recipient grant");
  receiver.rtiAmbassador().timeAdvanceRequest(*time1);
  sender.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions({&sender, &receiver}, [&] {
    return sender.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           receiver.federateSaveInitiations() == std::vector<std::wstring>{saveLabel};
  }, options, "two-member save initiation");
  sender.rtiAmbassador().federateSaveBegun();
  receiver.rtiAmbassador().federateSaveBegun();
  sender.rtiAmbassador().federateSaveComplete();
  receiver.rtiAmbassador().federateSaveComplete();
  waitForSessions({&sender, &receiver}, [&] {
    return sender.federationSavedCount() == 1U && receiver.federationSavedCount() == 1U;
  }, options, "two-member baseline saved");

  auto const discarded = sender.rtiAmbassador().sendDirectedInteraction(
      senderInteraction, target, rti::ParameterHandleValueMap{}, tag, *time6);
  require(discarded.isValid() && copyBytes(saved.encode()) != copyBytes(discarded.encode()),
          "Post-save designator reused the saved directed passel's handle");
  sender.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions({&sender, &receiver}, [&] {
    return sender.federateRestoreInitiations().size() == 1U &&
           receiver.federateRestoreInitiations().size() == 1U;
  }, options, "two-member restore initiation");
  sender.rtiAmbassador().federateRestoreComplete();
  receiver.rtiAmbassador().federateRestoreComplete();
  waitForSessions({&sender, &receiver}, [&] {
    return sender.federationRestoredCount() == 1U &&
           receiver.federationRestoredCount() == 1U;
  }, options, "two-member federation restored");

  requireException([&] { sender.rtiAmbassador().retract(discarded); },
                   L"InvalidMessageRetractionHandle", "discarded post-save designator");
  auto const fresh = sender.rtiAmbassador().sendDirectedInteraction(
      senderInteraction, target, rti::ParameterHandleValueMap{}, tag, *time6);
  require(fresh.isValid() && copyBytes(discarded.encode()) != copyBytes(fresh.encode()) &&
              copyBytes(saved.encode()) != copyBytes(fresh.encode()),
          "Post-restore allocation reused an old retraction designator");
  sender.rtiAmbassador().retract(fresh);
  requireException([&] { sender.rtiAmbassador().retract(fresh); },
                   L"MessageCanNoLongerBeRetracted", "fresh retraction terminal state");

  receiver.rtiAmbassador().flushQueueRequest(*time6);
  waitForSessions({&receiver}, [&] {
    return receiver.recorder().timedDirectedInteractions().size() == 1U &&
           receiver.recorder().flushQueueGrants().size() == 1U;
  }, options, "saved directed interaction and flush queue grant");
  auto const delivered = receiver.recorder().timedDirectedInteractions().front();
  require(delivered.interaction == receiverInteraction && delivered.object == target &&
              delivered.parameters.empty() && delivered.tag ==
                  std::vector<std::uint8_t>(tagBytes.begin(), tagBytes.end()) &&
              delivered.sentOrder == rti::TIMESTAMP &&
              delivered.receivedOrder == rti::TIMESTAMP &&
              delivered.retractionPresent &&
              delivered.retraction == copyBytes(saved.encode()) &&
              delivered.time == copyBytes(time6->encode()),
          "Fresh retraction aliased or suppressed the saved directed passel");
  sender.rtiAmbassador().retract(saved);
  waitForSessions({&receiver}, [&] {
    return receiver.recorder().retractions().size() == 1U;
  }, options, "saved directed retraction callback");
  require(receiver.recorder().retractions().front().valid &&
              receiver.recorder().retractions().front().encoded == copyBytes(saved.encode()),
          "Saved directed retraction callback carried the wrong designator");
  requireException([&] { sender.rtiAmbassador().retract(saved); },
                   L"MessageCanNoLongerBeRetracted", "saved retraction terminal state");

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

void scenarioRestoreTsoRetractionUniqueness(Options const& options,
                                            rti::CallbackModel model) {
  runRestoreTsoRetractionUniqueness(options, model);
}

void scenarioRestoreTsoRetractionUniquenessContract(Options const& options,
                                                    rti::CallbackModel model) {
  runRestoreTsoRetractionUniqueness(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
