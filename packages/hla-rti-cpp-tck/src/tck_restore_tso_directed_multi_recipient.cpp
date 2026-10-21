#include "tck_restore_tso_directed_multi_recipient.hpp"

#include <array>

namespace hla_rti_cpp_tck::detail {
namespace {

struct RecipientHandles {
  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interaction;
};

RecipientHandles subscribeRecipient(Session& recipient, Options const& options) {
  RecipientHandles handles{
      recipient.rtiAmbassador().getObjectClassHandle(options.objectClassName),
      {},
      recipient.rtiAmbassador().getInteractionClassHandle(options.interactionClassName)};
  handles.attribute = recipient.rtiAmbassador().getAttributeHandle(
      handles.objectClass, options.attributeName);
  require(handles.objectClass.isValid() && handles.attribute.isValid() &&
              handles.interaction.isValid(),
          "Adapter FOM lacks directed recipient object, attribute, or interaction class");
  recipient.rtiAmbassador().subscribeObjectClassAttributes(
      handles.objectClass, rti::AttributeHandleSet{handles.attribute});
  recipient.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      handles.objectClass, rti::InteractionClassHandleSet{handles.interaction}, true);
  return handles;
}

void checkDelivered(Session& recipient, RecipientHandles const& handles,
                    rti::ObjectInstanceHandle const& target,
                    rti::FederateHandle const& sender,
                    rti::TransportationTypeHandle const& reliable,
                    rti::MessageRetractionHandle const& saved,
                    rti::LogicalTime const& time8,
                    std::array<std::uint8_t, 4> const& tagBytes) {
  auto const deliveries = recipient.recorder().timedDirectedInteractions();
  require(deliveries.size() == 1U && recipient.recorder().flushQueueGrants().size() == 1U,
          "Restored directed passel or flush queue grant was missing");
  auto const& delivered = deliveries.front();
  require(delivered.interaction == handles.interaction && delivered.object == target &&
              delivered.parameters.empty() &&
              delivered.tag == std::vector<std::uint8_t>(tagBytes.begin(), tagBytes.end()) &&
              delivered.transportation == reliable && delivered.producer == sender &&
              delivered.time == copyBytes(time8.encode()) &&
              delivered.sentOrder == rti::TIMESTAMP &&
              delivered.receivedOrder == rti::TIMESTAMP &&
              delivered.retractionPresent &&
              delivered.retraction == copyBytes(saved.encode()) &&
              recipient.recorder().callbackOrder() ==
                  std::vector<std::string>{"directed", "flush-grant"},
          "Restored directed callback changed public fields or ordering");
}

void runRestoreTsoDirectedMultiRecipient(Options const& options,
                                         rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Multi-recipient directed restore has native evidence only for evoked callbacks");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Multi-recipient directed restore requires adapter FOM and time implementation");
  Session publisher(options, model, "owner");
  Session first(options, model, "member-a");
  Session second(options, model, "member-b");
  auto const federation = federationName(options, "restore-tso-directed-multi-recipient");
  std::wstring const saveLabel = L"tck-directed-tso-multi-recipient-restore";
  std::array<std::uint8_t, 4> const tagBytes{'M', 'R', 'F', '2'};
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
  auto const objectClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = publisher.rtiAmbassador().getAttributeHandle(
      objectClass, options.attributeName);
  auto const interaction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(objectClass.isValid() && attribute.isValid() && interaction.isValid(),
          "Adapter FOM lacks directed publisher object, attribute, or interaction class");
  publisher.rtiAmbassador().publishObjectClassAttributes(
      objectClass, rti::AttributeHandleSet{attribute});
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      objectClass, rti::InteractionClassHandleSet{interaction});
  publisher.rtiAmbassador().changeInteractionOrderType(interaction, rti::TIMESTAMP);
  auto const firstHandles = subscribeRecipient(first, options);
  auto const secondHandles = subscribeRecipient(second, options);
  auto const target = publisher.rtiAmbassador().registerObjectInstance(objectClass);
  require(target.isValid(), "Directed multi-recipient target was invalid");
  waitForSessions({&first, &second}, [&] {
    return first.recorder().hasDiscovery(target) && second.recorder().hasDiscovery(target);
  }, options, "both directed target discoveries");

  first.rtiAmbassador().enableTimeConstrained();
  second.rtiAmbassador().enableTimeConstrained();
  first.pump();
  second.pump();
  auto const time = makeTimeContext(publisher);
  auto const time1 = timeAfter(*time.factory, *time.initial, *time.epsilon, 1U);
  auto const time8 = timeAfter(*time.factory, *time.initial, *time.epsilon, 8U);
  auto lookahead = time.factory->makeZero();
  require(lookahead != nullptr, "Time factory returned no lookahead interval");
  for (unsigned step = 0U; step != 5U; ++step) *lookahead += *time.epsilon;
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitForSessions({&publisher}, [&] {
    return publisher.recorder().timeRegulationEnabled().size() == 1U;
  }, options, "publisher time-regulation enablement");

  auto const saved = publisher.rtiAmbassador().sendDirectedInteraction(
      interaction, target, rti::ParameterHandleValueMap{}, tag, *time8);
  require(saved.isValid() && first.recorder().timedDirectedInteractions().empty() &&
              second.recorder().timedDirectedInteractions().empty(),
          "Timestamp-eight directed passel was not queued for both recipients");
  first.rtiAmbassador().timeAdvanceRequest(*time1);
  second.rtiAmbassador().timeAdvanceRequest(*time1);
  publisher.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions({&publisher, &first, &second}, [&] {
    return publisher.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           first.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           second.federateSaveInitiations() == std::vector<std::wstring>{saveLabel};
  }, options, "three-member save initiation");
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

  auto const discarded = publisher.rtiAmbassador().sendDirectedInteraction(
      interaction, target, rti::ParameterHandleValueMap{}, tag, *time8);
  require(discarded.isValid() && copyBytes(saved.encode()) != copyBytes(discarded.encode()),
          "Post-save directed designator reused the saved handle");
  publisher.rtiAmbassador().retract(discarded);
  first.pump();
  second.pump();
  require(first.recorder().timedDirectedInteractions().empty() &&
              second.recorder().timedDirectedInteractions().empty(),
          "Post-save retracted passel reached a recipient");
  requireException([&] { publisher.rtiAmbassador().retract(discarded); },
                   L"MessageCanNoLongerBeRetracted", "post-save terminal designator");
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
  require(first.recorder().timedDirectedInteractions().empty() &&
              second.recorder().timedDirectedInteractions().empty(),
          "Restored directed passel was delivered before a recipient FQR");
  requireException([&] { publisher.rtiAmbassador().retract(discarded); },
                   L"InvalidMessageRetractionHandle", "discarded post-save designator");

  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();
  first.rtiAmbassador().flushQueueRequest(*time8);
  waitForSessions({&first}, [&] {
    return first.recorder().timedDirectedInteractions().size() == 1U &&
           first.recorder().flushQueueGrants().size() == 1U;
  }, options, "first restored directed recipient FQR");
  require(second.recorder().timedDirectedInteractions().empty(),
          "Second recipient's restored passel was released by the first FQR");
  second.rtiAmbassador().flushQueueRequest(*time8);
  waitForSessions({&second}, [&] {
    return second.recorder().timedDirectedInteractions().size() == 1U &&
           second.recorder().flushQueueGrants().size() == 1U;
  }, options, "second restored directed recipient FQR");
  auto const reliable = publisher.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  checkDelivered(first, firstHandles, target, publisherHandle, reliable, saved,
                 *time8, tagBytes);
  checkDelivered(second, secondHandles, target, publisherHandle, reliable, saved,
                 *time8, tagBytes);
  publisher.rtiAmbassador().retract(saved);
  waitForSessions({&first, &second}, [&] {
    return first.recorder().retractions().size() == 1U &&
           second.recorder().retractions().size() == 1U;
  }, options, "both restored directed request-retraction callbacks");
  for (Session* recipient : {&first, &second}) {
    auto const retractions = recipient->recorder().retractions();
    require(retractions.front().valid &&
                retractions.front().encoded == copyBytes(saved.encode()),
            "A recipient received the wrong restored retraction designator");
  }
  requireException([&] { publisher.rtiAmbassador().retract(saved); },
                   L"MessageCanNoLongerBeRetracted", "saved terminal designator");

  first.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      firstHandles.objectClass, rti::InteractionClassHandleSet{firstHandles.interaction});
  second.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      secondHandles.objectClass, rti::InteractionClassHandleSet{secondHandles.interaction});
  first.rtiAmbassador().unsubscribeObjectClassAttributes(
      firstHandles.objectClass, rti::AttributeHandleSet{firstHandles.attribute});
  second.rtiAmbassador().unsubscribeObjectClassAttributes(
      secondHandles.objectClass, rti::AttributeHandleSet{secondHandles.attribute});
  first.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  first.disconnect();
  second.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioRestoreTsoDirectedMultiRecipient(Options const& options,
                                              rti::CallbackModel model) {
  runRestoreTsoDirectedMultiRecipient(options, model);
}

void scenarioRestoreTsoDirectedMultiRecipientContract(Options const& options,
                                                      rti::CallbackModel model) {
  runRestoreTsoDirectedMultiRecipient(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
