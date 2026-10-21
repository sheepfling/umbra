#include "tck_restore_live_tso_directed_interaction.hpp"

#include <array>

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestoreLiveTsoDirectedInteraction(Options const& options,
                                          rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Live directed restore has native evidence only for evoked callbacks");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Live directed restore requires adapter FOM and time implementation");
  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "restore-live-tso-directed");
  std::wstring const saveLabel = L"tck-live-directed-retraction-baseline";
  std::array<std::uint8_t, 4> const tagBytes{'S', 'D', 'I', '6'};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  publisher.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType + L"-publisher", federation);
  receiver.join(options.memberFederateName, options.federateType + L"-subscriber", federation);
  auto const publisherHandle = publisher.federateHandle();
  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass, options.attributeName);
  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(publisherClass.isValid() && receiverClass.isValid() &&
              publisherAttribute.isValid() && receiverAttribute.isValid() &&
              publisherInteraction.isValid() && receiverInteraction.isValid(),
          "Adapter FOM lacks directed object, attribute, or interaction class");
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, rti::AttributeHandleSet{publisherAttribute});
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass, rti::AttributeHandleSet{receiverAttribute});
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherClass, rti::InteractionClassHandleSet{publisherInteraction});
  receiver.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      receiverClass, rti::InteractionClassHandleSet{receiverInteraction}, true);
  publisher.rtiAmbassador().changeInteractionOrderType(publisherInteraction, rti::TIMESTAMP);
  auto const target = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(target.isValid(), "Directed target was invalid");
  waitForSessions({&receiver}, [&] { return receiver.recorder().hasDiscovery(target); },
                  options, "directed target discovery");
  receiver.rtiAmbassador().enableTimeConstrained();
  receiver.pump();
  auto const time = makeTimeContext(publisher);
  auto const time1 = timeAfter(*time.factory, *time.initial, *time.epsilon, 1U);
  auto const time6 = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  auto lookahead = time.factory->makeZero();
  require(lookahead != nullptr, "Time factory returned no lookahead interval");
  for (unsigned step = 0U; step != 5U; ++step) *lookahead += *time.epsilon;
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitForSessions({&publisher}, [&] {
    return publisher.recorder().timeRegulationEnabled().size() == 1U;
  }, options, "publisher time-regulation enablement");

  auto const saved = publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction, target, rti::ParameterHandleValueMap{}, tag, *time6);
  require(saved.isValid() && receiver.recorder().timedDirectedInteractions().empty(),
          "Timestamp-six directed interaction was not queued before save");
  receiver.rtiAmbassador().timeAdvanceRequest(*time1);
  publisher.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions({&publisher, &receiver}, [&] {
    return publisher.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           receiver.federateSaveInitiations() == std::vector<std::wstring>{saveLabel};
  }, options, "two-member directed save initiation");
  publisher.rtiAmbassador().federateSaveBegun();
  receiver.rtiAmbassador().federateSaveBegun();
  publisher.rtiAmbassador().federateSaveComplete();
  receiver.rtiAmbassador().federateSaveComplete();
  waitForSessions({&publisher, &receiver}, [&] {
    return publisher.federationSavedCount() == 1U && receiver.federationSavedCount() == 1U;
  }, options, "directed federation saved");

  publisher.rtiAmbassador().retract(saved);
  receiver.pump();
  require(receiver.recorder().timedDirectedInteractions().empty() &&
              receiver.recorder().retractions().empty(),
          "Post-save retraction leaked a directed callback");
  requireException([&] { publisher.rtiAmbassador().retract(saved); },
                   L"MessageCanNoLongerBeRetracted", "post-save terminal directed handle");
  publisher.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions({&publisher, &receiver}, [&] {
    return publisher.federateRestoreInitiations().size() == 1U &&
           receiver.federateRestoreInitiations().size() == 1U;
  }, options, "two-member directed restore initiation");
  publisher.rtiAmbassador().federateRestoreComplete();
  receiver.rtiAmbassador().federateRestoreComplete();
  waitForSessions({&publisher, &receiver}, [&] {
    return publisher.federationRestoredCount() == 1U &&
           receiver.federationRestoredCount() == 1U;
  }, options, "directed federation restored");
  require(receiver.recorder().timedDirectedInteractions().empty(),
          "Restored directed interaction arrived before receiver FQR");

  receiver.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().flushQueueRequest(*time6);
  waitForSessions({&receiver}, [&] {
    return receiver.recorder().timedDirectedInteractions().size() == 1U &&
           receiver.recorder().flushQueueGrants().size() == 1U;
  }, options, "restored directed delivery and flush queue grant");
  auto const delivered = receiver.recorder().timedDirectedInteractions().front();
  auto const reliable = publisher.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(delivered.interaction == receiverInteraction && delivered.object == target &&
              delivered.parameters.empty() &&
              delivered.tag == std::vector<std::uint8_t>(tagBytes.begin(), tagBytes.end()) &&
              delivered.transportation == reliable && delivered.producer == publisherHandle &&
              delivered.time == copyBytes(time6->encode()) &&
              delivered.sentOrder == rti::TIMESTAMP &&
              delivered.receivedOrder == rti::TIMESTAMP &&
              delivered.retractionPresent &&
              delivered.retraction == copyBytes(saved.encode()) &&
              receiver.recorder().callbackOrder() ==
                  std::vector<std::string>{"directed", "flush-grant"},
          "Restored directed callback changed public fields or ordering");
  publisher.rtiAmbassador().retract(saved);
  waitForSessions({&receiver}, [&] {
    return receiver.recorder().retractions().size() == 1U;
  }, options, "restored directed request-retraction callback");
  auto const retraction = receiver.recorder().retractions().front();
  require(retraction.valid && retraction.encoded == copyBytes(saved.encode()),
          "Restored directed retraction callback returned a different handle");
  requireException([&] { publisher.rtiAmbassador().retract(saved); },
                   L"MessageCanNoLongerBeRetracted", "restored terminal directed handle");

  receiver.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      receiverClass, rti::InteractionClassHandleSet{receiverInteraction});
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass, rti::AttributeHandleSet{receiverAttribute});
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioRestoreLiveTsoDirectedInteraction(Options const& options,
                                                rti::CallbackModel model) {
  runRestoreLiveTsoDirectedInteraction(options, model);
}

void scenarioRestoreLiveTsoDirectedInteractionContract(Options const& options,
                                                        rti::CallbackModel model) {
  runRestoreLiveTsoDirectedInteraction(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
