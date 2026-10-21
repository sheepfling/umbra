#include "tck_timed_save_flush_queue_admission.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runTimedSaveFlushQueueAdmission(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Flush Queue timed save requires evoked callbacks");
  require(!options.fom.empty() && !options.interactionClassName.empty() &&
              !options.parameterName.empty(),
          "Flush Queue timed save requires adapter-supplied FOM and interaction names");
  require(!options.logicalTimeImplementationName.empty(),
          "Flush Queue timed save requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "timed-save-flush-queue-admission");
  std::wstring const saveLabel = L"tck-timed-save-flush-queue-admission";
  owner.connect();
  receiver.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  receiver.join(options.memberFederateName + L"-receiver", options.federateType,
                federation);

  auto const ownerInteraction = owner.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const ownerParameter = owner.rtiAmbassador().getParameterHandle(
      ownerInteraction, options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction, options.parameterName);
  require(ownerInteraction.isValid() && receiverInteraction.isValid() &&
              ownerParameter.isValid() && receiverParameter.isValid(),
          "Flush Queue timed save resolved an invalid FOM handle");
  owner.rtiAmbassador().publishInteractionClass(ownerInteraction);
  owner.rtiAmbassador().changeInteractionOrderType(ownerInteraction, rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto const time = makeTimeContext(owner);
  require(time.factory->getName() == makeTimeContext(receiver).factory->getName(),
          "Flush Queue timed save members selected different logical-time factories");
  auto const ownerFirstTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 4U);
  auto const saveTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const laterTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  auto const ownerFirstEncoded = encodeTime(*ownerFirstTime);
  auto const saveEncoded = encodeTime(*saveTime);
  auto const laterEncoded = encodeTime(*laterTime);
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(receiver,
          [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "Flush Queue constrained-role enablement");
  owner.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(owner,
          [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
          options, "Flush Queue regulator enablement");
  owner.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();

  std::vector<std::uint8_t> const payload{0xF0U, 0x51U};
  std::vector<std::uint8_t> const tagBytes{0x46U, 0x51U, 0x52U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(ownerParameter,
                     rti::VariableLengthData(payload.data(), payload.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = owner.rtiAmbassador().sendInteraction(
      ownerInteraction, parameters, tag, *saveTime);
  require(retraction.isValid(),
          "Flush Queue timed save send returned an invalid retraction handle");
  owner.rtiAmbassador().requestFederationSave(saveLabel, *saveTime);
  owner.rtiAmbassador().timeAdvanceRequest(*ownerFirstTime);
  receiver.rtiAmbassador().flushQueueRequest(*saveTime);

  waitFor(receiver,
          [&] {
            return receiver.recorder().timedInteractions().size() == 1U &&
                receiver.recorder().flushQueueGrants().size() == 1U;
          }, options, "Flush Queue equal-boundary interaction and grant");
  auto const deliveries = receiver.recorder().timedInteractions();
  require(deliveries.size() == 1U &&
              deliveries.front().interaction == receiverInteraction &&
              deliveries.front().time == saveEncoded &&
              deliveries.front().parameters.size() == 1U &&
              copyBytes(deliveries.front().parameters.at(receiverParameter)) == payload &&
              deliveries.front().tag == tagBytes &&
              deliveries.front().receivedOrder == rti::TIMESTAMP,
          "Flush Queue recipient did not receive timestamped interaction at save time");
  require(receiver.federateSaveInitiations().empty() &&
              receiver.recorder().flushQueueGrants().front().time == saveEncoded &&
              receiver.recorder().callbackOrder() ==
                  std::vector<std::string>{"interaction", "flush-grant"} &&
              owner.recorder().callbackOrder().empty(),
          "Equal actual Flush Queue grant incorrectly initiated the save");

  waitFor(owner,
          [&] { return owner.recorder().timeAdvanceGrants().size() == 1U; },
          options, "Flush Queue regulator first grant");
  require(owner.recorder().timeAdvanceGrants().front().encoded == ownerFirstEncoded &&
              owner.recorder().callbackOrder() == std::vector<std::string>{"grant"},
          "Regulator did not receive its preliminary time-4 grant");
  owner.rtiAmbassador().timeAdvanceRequest(*saveTime);
  receiver.rtiAmbassador().flushQueueRequest(*laterTime);

  waitFor(receiver,
          [&] {
            return receiver.federateSaveInitiations().size() == 1U &&
                receiver.recorder().flushQueueGrants().size() == 2U;
          }, options, "Flush Queue strict-later save initiation and grant");
  require(receiver.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              receiver.federateSaveTimes().size() == 1U &&
              receiver.federateSaveTimes().front().encoded == saveEncoded &&
              receiver.recorder().flushQueueGrants().back().time == laterEncoded &&
              receiver.recorder().callbackOrder() ==
                  std::vector<std::string>{"interaction", "flush-grant",
                                           "federate-save-initiate", "flush-grant"} &&
              owner.recorder().callbackOrder() == std::vector<std::string>{"grant"},
          "Strict-later actual Flush Queue grant did not initiate save first");

  waitFor(owner,
          [&] { return owner.federateSaveInitiations().size() == 1U; },
          options, "Flush Queue regulator save initiation");
  require(owner.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              owner.federateSaveTimes().size() == 1U &&
              owner.federateSaveTimes().front().encoded == saveEncoded,
          "Regulator did not receive the matching timed save initiation");

  for (auto* member : {&owner, &receiver}) {
    member->rtiAmbassador().federateSaveBegun();
  }
  for (auto* member : {&owner, &receiver}) {
    member->rtiAmbassador().federateSaveComplete();
  }
  waitForSessions(
      {&owner, &receiver},
      [&] {
        return owner.federationSavedCount() == 1U &&
            receiver.federationSavedCount() == 1U;
      }, options, "two-member Flush Queue save completion");
  require(owner.federationNotSavedReasons().empty() &&
              receiver.federationNotSavedReasons().empty(),
          "Flush Queue participant received Federation Not Saved");

  receiver.rtiAmbassador().disableTimeConstrained();
  owner.rtiAmbassador().disableTimeRegulation();
  receiver.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioTimedSaveFlushQueueAdmission(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveFlushQueueAdmission(options, model);
}

void scenarioTimedSaveFlushQueueAdmissionContract(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveFlushQueueAdmission(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
