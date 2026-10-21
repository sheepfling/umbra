#include "tck_restore_live_tso_interaction_multi_recipient.hpp"

#include <array>

namespace hla_rti_cpp_tck::detail {
namespace {

struct RecipientHandles {
  rti::InteractionClassHandle interaction;
  rti::ParameterHandle parameter;
};

RecipientHandles subscribe(Session& receiver, Options const& options) {
  auto const interaction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = receiver.rtiAmbassador().getParameterHandle(
      interaction, options.parameterName);
  require(interaction.isValid() && parameter.isValid(),
          "Adapter FOM lacks interaction class or parameter for recipient");
  receiver.rtiAmbassador().subscribeInteractionClass(interaction);
  return {interaction, parameter};
}

void checkDelivered(Session& receiver, RecipientHandles const& handles,
                    rti::FederateHandle const& publisherHandle,
                    rti::MessageRetractionHandle const& saved,
                    rti::LogicalTime const& time7, rti::LogicalTime const& time5,
                    std::array<std::uint8_t, 5> const& payload,
                    std::array<std::uint8_t, 7> const& tag) {
  auto const deliveries = receiver.recorder().timedInteractions();
  auto const grants = receiver.recorder().flushQueueGrants();
  require(deliveries.size() == 1U && grants.size() == 1U,
          "Recipient did not receive exactly one restored interaction and FQG");
  auto const& delivered = deliveries.front();
  require(delivered.interaction == handles.interaction &&
              delivered.parameters.size() == 1U &&
              delivered.parameters.count(handles.parameter) == 1U &&
              copyBytes(delivered.parameters.at(handles.parameter)) ==
                  std::vector<std::uint8_t>(payload.begin(), payload.end()) &&
              delivered.tag == std::vector<std::uint8_t>(tag.begin(), tag.end()) &&
              delivered.producer == publisherHandle &&
              delivered.time == copyBytes(time7.encode()) &&
              delivered.sentOrder == rti::TIMESTAMP &&
              delivered.receivedOrder == rti::TIMESTAMP &&
              delivered.retractionPresent &&
              delivered.retraction == copyBytes(saved.encode()) &&
              grants.front().time == copyBytes(time5.encode()) &&
              grants.front().optimisticTime == copyBytes(time7.encode()) &&
              receiver.recorder().callbackOrder() ==
                  std::vector<std::string>{"interaction", "flush-grant"},
          "Restored recipient interaction, grant, or callback ordering changed");
}

void runRestoreLiveTsoInteractionMultiRecipient(Options const& options,
                                                rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Multi-recipient ordinary restore has native evidence only for evoked callbacks");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Multi-recipient ordinary restore requires adapter FOM and time implementation");
  Session publisher(options, model, "owner");
  Session first(options, model, "member-a");
  Session second(options, model, "member-b");
  auto const federation = federationName(options, "restore-live-tso-interaction-multi");
  std::wstring const saveLabel = L"tck-live-tso-interaction-multi-baseline";
  std::array<std::uint8_t, 5> const payload{'M', 'U', 'L', 'T', 'I'};
  std::array<std::uint8_t, 7> const tagBytes{'R', 'E', 'S', 'T', 'O', 'R', 'E'};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  publisher.connect();
  first.connect();
  second.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType + L"-publisher", federation);
  first.join(options.memberFederateName + L"-a", options.federateType + L"-subscriber",
             federation);
  second.join(options.memberFederateName + L"-b", options.federateType + L"-subscriber",
              federation);
  auto const publisherHandle = publisher.federateHandle();
  auto const interaction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = publisher.rtiAmbassador().getParameterHandle(
      interaction, options.parameterName);
  require(interaction.isValid() && parameter.isValid(),
          "Adapter FOM lacks publisher interaction class or parameter");
  rti::ParameterHandleValueMap values;
  values.emplace(parameter, rti::VariableLengthData(payload.data(), payload.size()));
  publisher.rtiAmbassador().publishInteractionClass(interaction);
  publisher.rtiAmbassador().changeInteractionOrderType(interaction, rti::TIMESTAMP);
  auto const firstHandles = subscribe(first, options);
  auto const secondHandles = subscribe(second, options);
  first.rtiAmbassador().enableTimeConstrained();
  second.rtiAmbassador().enableTimeConstrained();
  first.pump();
  second.pump();
  auto const time = makeTimeContext(publisher);
  auto const time1 = timeAfter(*time.factory, *time.initial, *time.epsilon, 1U);
  auto const time5 = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const time7 = timeAfter(*time.factory, *time.initial, *time.epsilon, 7U);
  auto const time10 = timeAfter(*time.factory, *time.initial, *time.epsilon, 10U);
  auto lookahead = time.factory->makeZero();
  require(lookahead != nullptr, "Time factory returned no lookahead interval");
  for (unsigned step = 0U; step != 5U; ++step) *lookahead += *time.epsilon;
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitForSessions({&publisher}, [&] {
    return publisher.recorder().timeRegulationEnabled().size() == 1U;
  }, options, "publisher time-regulation enablement");

  auto const saved = publisher.rtiAmbassador().sendInteraction(
      interaction, values, tag, *time7);
  require(saved.isValid() && first.recorder().timedInteractions().empty() &&
              second.recorder().timedInteractions().empty(),
          "Timestamp-seven interaction was not queued for both recipients");
  first.rtiAmbassador().timeAdvanceRequest(*time1);
  second.rtiAmbassador().timeAdvanceRequest(*time1);
  publisher.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           first.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           second.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           first.recorder().timeAdvanceGrants().size() == 1U &&
           second.recorder().timeAdvanceGrants().size() == 1U;
  }, options, "three-member save initiation and time-one grants");
  require(first.recorder().timeAdvanceGrants().front().encoded == copyBytes(time1->encode()) &&
              second.recorder().timeAdvanceGrants().front().encoded == copyBytes(time1->encode()),
          "A constrained recipient did not reach time one before the save");
  publisher.rtiAmbassador().federateSaveBegun();
  first.rtiAmbassador().federateSaveBegun();
  second.rtiAmbassador().federateSaveBegun();
  publisher.rtiAmbassador().federateSaveComplete();
  first.rtiAmbassador().federateSaveComplete();
  second.rtiAmbassador().federateSaveComplete();
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federationSavedCount() == 1U &&
           first.federationSavedCount() == 1U && second.federationSavedCount() == 1U;
  }, options, "three-member federation saved");
  require(first.recorder().timedInteractions().empty() &&
              second.recorder().timedInteractions().empty(),
          "A queued interaction arrived while saving");

  publisher.rtiAmbassador().retract(saved);
  first.pump();
  second.pump();
  require(first.recorder().timedInteractions().empty() &&
              second.recorder().timedInteractions().empty(),
          "Post-save retract leaked a queued interaction");
  requireException([&] { publisher.rtiAmbassador().retract(saved); },
                   L"MessageCanNoLongerBeRetracted", "post-save terminal handle");
  publisher.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federateRestoreInitiations().size() == 1U &&
           first.federateRestoreInitiations().size() == 1U &&
           second.federateRestoreInitiations().size() == 1U;
  }, options, "three-member restore initiation");
  publisher.rtiAmbassador().federateRestoreComplete();
  first.rtiAmbassador().federateRestoreComplete();
  second.rtiAmbassador().federateRestoreComplete();
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federationRestoredCount() == 1U &&
           first.federationRestoredCount() == 1U &&
           second.federationRestoredCount() == 1U;
  }, options, "three-member federation restored");

  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();
  first.rtiAmbassador().flushQueueRequest(*time10);
  waitForSessions({&first}, [&] {
    return first.recorder().timedInteractions().size() == 1U &&
           first.recorder().flushQueueGrants().size() == 1U;
  }, options, "first restored interaction and flush queue grant");
  require(second.recorder().timedInteractions().empty() &&
              second.recorder().flushQueueGrants().empty() &&
              second.recorder().callbackOrder().empty(),
          "First FQR prematurely released the second recipient");
  second.rtiAmbassador().flushQueueRequest(*time10);
  waitForSessions({&second}, [&] {
    return second.recorder().timedInteractions().size() == 1U &&
           second.recorder().flushQueueGrants().size() == 1U;
  }, options, "second restored interaction and flush queue grant");
  checkDelivered(first, firstHandles, publisherHandle, saved, *time7, *time5,
                 payload, tagBytes);
  checkDelivered(second, secondHandles, publisherHandle, saved, *time7, *time5,
                 payload, tagBytes);

  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();
  publisher.rtiAmbassador().retract(saved);
  waitForSessions({&first, &second}, [&] {
    return first.recorder().retractions().size() == 1U &&
           second.recorder().retractions().size() == 1U;
  }, options, "both restored request-retraction callbacks");
  for (Session* recipient : {&first, &second}) {
    auto const retraction = recipient->recorder().retractions().front();
    require(retraction.valid && retraction.encoded == copyBytes(saved.encode()) &&
                recipient->recorder().callbackOrder() ==
                    std::vector<std::string>{"request-retraction"},
            "Restored retraction callback differed between recipients");
  }
  requireException([&] { publisher.rtiAmbassador().retract(saved); },
                   L"MessageCanNoLongerBeRetracted", "restored terminal handle");

  first.rtiAmbassador().unsubscribeInteractionClass(firstHandles.interaction);
  second.rtiAmbassador().unsubscribeInteractionClass(secondHandles.interaction);
  first.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  first.disconnect();
  second.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioRestoreLiveTsoInteractionMultiRecipient(Options const& options,
                                                     rti::CallbackModel model) {
  runRestoreLiveTsoInteractionMultiRecipient(options, model);
}

void scenarioRestoreLiveTsoInteractionMultiRecipientContract(Options const& options,
                                                             rti::CallbackModel model) {
  runRestoreLiveTsoInteractionMultiRecipient(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
