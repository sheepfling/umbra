#include "tck_restore_live_tso_retraction_record.hpp"

#include <array>

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestoreLiveTsoRetractionRecord(Options const& options,
                                       rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Live TSO retraction-record restore has native evidence only for evoked callbacks");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Live retraction restore requires adapter FOM and time implementation");
  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "restore-live-tso-retraction-record");
  std::wstring const saveLabel = L"tck-live-tso-retraction-baseline";
  std::array<std::uint8_t, 2> const payloadBytes{'Z', 'R'};
  std::array<std::uint8_t, 3> const tagBytes{'S', 'N', 'P'};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  publisher.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType + L"-publisher", federation);
  receiver.join(options.memberFederateName, options.federateType + L"-subscriber", federation);
  auto const publisherHandle = publisher.federateHandle();
  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction, options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction, options.parameterName);
  require(publisherInteraction.isValid() && receiverInteraction.isValid() &&
              publisherParameter.isValid() && receiverParameter.isValid(),
          "Adapter FOM lacks live retraction interaction class or parameter");
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(publisherParameter,
                     rti::VariableLengthData(payloadBytes.data(), payloadBytes.size()));
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(publisherInteraction, rti::TIMESTAMP);
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

  auto const saved = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction, parameters, tag, *time6);
  require(saved.isValid() && receiver.recorder().timedInteractions().empty(),
          "Timestamp-six interaction was not queued before the save");
  receiver.rtiAmbassador().timeAdvanceRequest(*time1);
  publisher.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions({&publisher, &receiver}, [&] {
    return publisher.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
           receiver.federateSaveInitiations() == std::vector<std::wstring>{saveLabel};
  }, options, "live retraction two-member save initiation");
  publisher.rtiAmbassador().federateSaveBegun();
  receiver.rtiAmbassador().federateSaveBegun();
  publisher.rtiAmbassador().federateSaveComplete();
  receiver.rtiAmbassador().federateSaveComplete();
  waitForSessions({&publisher, &receiver}, [&] {
    return publisher.federationSavedCount() == 1U &&
           receiver.federationSavedCount() == 1U;
  }, options, "live retraction baseline saved");

  publisher.rtiAmbassador().retract(saved);
  receiver.pump();
  require(receiver.recorder().timedInteractions().empty() &&
              receiver.recorder().retractions().empty(),
          "Post-save retraction leaked a queued callback");
  requireException([&] { publisher.rtiAmbassador().retract(saved); },
                   L"MessageCanNoLongerBeRetracted", "post-save terminal designator");
  publisher.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions({&publisher, &receiver}, [&] {
    return publisher.federateRestoreInitiations().size() == 1U &&
           receiver.federateRestoreInitiations().size() == 1U;
  }, options, "live retraction two-member restore initiation");
  publisher.rtiAmbassador().federateRestoreComplete();
  receiver.rtiAmbassador().federateRestoreComplete();
  waitForSessions({&publisher, &receiver}, [&] {
    return publisher.federationRestoredCount() == 1U &&
           receiver.federationRestoredCount() == 1U;
  }, options, "live retraction federation restored");
  require(receiver.recorder().timedInteractions().empty(),
          "Restored interaction arrived before its recipient FQR");

  receiver.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().flushQueueRequest(*time6);
  waitForSessions({&receiver}, [&] {
    return receiver.recorder().timedInteractions().size() == 1U &&
           receiver.recorder().flushQueueGrants().size() == 1U;
  }, options, "restored live interaction and flush queue grant");
  auto const delivered = receiver.recorder().timedInteractions().front();
  require(delivered.interaction == receiverInteraction &&
              delivered.parameters.size() == 1U &&
              delivered.parameters.count(receiverParameter) == 1U &&
              copyBytes(delivered.parameters.at(receiverParameter)) ==
                  std::vector<std::uint8_t>(payloadBytes.begin(), payloadBytes.end()) &&
              delivered.tag == std::vector<std::uint8_t>(tagBytes.begin(), tagBytes.end()) &&
              delivered.producer == publisherHandle &&
              delivered.time == copyBytes(time6->encode()) &&
              delivered.sentOrder == rti::TIMESTAMP &&
              delivered.receivedOrder == rti::TIMESTAMP &&
              delivered.retractionPresent &&
              delivered.retraction == copyBytes(saved.encode()) &&
              receiver.recorder().callbackOrder() ==
                  std::vector<std::string>{"interaction", "flush-grant"},
          "Restored live interaction changed payload, designator, or callback ordering");
  publisher.rtiAmbassador().retract(saved);
  waitForSessions({&receiver}, [&] {
    return receiver.recorder().retractions().size() == 1U;
  }, options, "restored live request-retraction callback");
  auto const retraction = receiver.recorder().retractions().front();
  require(retraction.valid && retraction.encoded == copyBytes(saved.encode()),
          "Restored live retraction callback returned a different designator");
  requireException([&] { publisher.rtiAmbassador().retract(saved); },
                   L"MessageCanNoLongerBeRetracted", "restored terminal designator");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioRestoreLiveTsoRetractionRecord(Options const& options,
                                            rti::CallbackModel model) {
  runRestoreLiveTsoRetractionRecord(options, model);
}

void scenarioRestoreLiveTsoRetractionRecordContract(Options const& options,
                                                    rti::CallbackModel model) {
  runRestoreLiveTsoRetractionRecord(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
