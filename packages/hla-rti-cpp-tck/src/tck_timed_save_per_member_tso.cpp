#include "tck_timed_save_per_member_tso.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runTimedSavePerMemberTso(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Per-member TSO timed save requires evoked callbacks");
  require(!options.fom.empty() && !options.interactionClassName.empty() &&
              !options.parameterName.empty(),
          "Per-member TSO timed save requires adapter-supplied FOM and interaction names");
  require(!options.logicalTimeImplementationName.empty(),
          "Per-member TSO timed save requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session ready(options, model, "member");
  Session delayed(options, model, "member");
  auto const federation = federationName(options, "timed-save-per-member-tso");
  std::wstring const saveLabel = L"tck-timed-save-per-member-tso";
  owner.connect();
  ready.connect();
  delayed.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  ready.join(options.memberFederateName + L"-ready", options.federateType, federation);
  delayed.join(options.memberFederateName + L"-delayed", options.federateType, federation);

  auto const ownerInteraction = owner.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const delayedInteraction = delayed.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const ownerParameter = owner.rtiAmbassador().getParameterHandle(
      ownerInteraction, options.parameterName);
  auto const delayedParameter = delayed.rtiAmbassador().getParameterHandle(
      delayedInteraction, options.parameterName);
  require(ownerInteraction.isValid() && delayedInteraction.isValid() &&
              ownerParameter.isValid() && delayedParameter.isValid(),
          "Per-member TSO timed save resolved an invalid FOM handle");
  owner.rtiAmbassador().publishInteractionClass(ownerInteraction);
  owner.rtiAmbassador().changeInteractionOrderType(ownerInteraction, rti::TIMESTAMP);
  delayed.rtiAmbassador().subscribeInteractionClass(delayedInteraction, true);

  auto const time = makeTimeContext(owner);
  require(time.factory->getName() == makeTimeContext(ready).factory->getName() &&
              time.factory->getName() == makeTimeContext(delayed).factory->getName(),
          "Per-member TSO timed save members selected different logical-time factories");
  auto const saveTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const saveEncoded = encodeTime(*saveTime);
  ready.rtiAmbassador().enableTimeConstrained();
  waitFor(ready,
          [&] { return ready.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "ready per-member TSO constrained-role enablement");
  delayed.rtiAmbassador().enableTimeConstrained();
  waitFor(delayed,
          [&] { return delayed.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "delayed per-member TSO constrained-role enablement");
  owner.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(owner,
          [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
          options, "per-member TSO regulator enablement");
  owner.recorder().clearCallbackOrder();
  ready.recorder().clearCallbackOrder();
  delayed.recorder().clearCallbackOrder();

  std::vector<std::uint8_t> const payload{0x50U, 0x45U, 0x52U};
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x53U, 0x4FU};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(ownerParameter,
                     rti::VariableLengthData(payload.data(), payload.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = owner.rtiAmbassador().sendInteraction(
      ownerInteraction, parameters, tag, *saveTime);
  require(retraction.isValid(),
          "Per-member TSO timed save send returned an invalid retraction handle");
  owner.rtiAmbassador().requestFederationSave(saveLabel, *saveTime);
  ready.rtiAmbassador().timeAdvanceRequest(*saveTime);
  delayed.rtiAmbassador().timeAdvanceRequest(*saveTime);
  owner.rtiAmbassador().timeAdvanceRequest(*saveTime);

  waitFor(ready,
          [&] {
            return ready.federateSaveInitiations().size() == 1U &&
                ready.recorder().timeAdvanceGrants().size() == 1U;
          }, options, "ready per-member TSO save initiation and grant");
  require(ready.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              ready.federateSaveTimes().size() == 1U &&
              ready.federateSaveTimes().front().encoded == saveEncoded &&
              ready.recorder().timeAdvanceGrants().front().encoded == saveEncoded &&
              ready.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "grant"},
          "Ready member did not initiate save before grant 5");
  require(delayed.recorder().timedInteractions().empty() &&
              delayed.federateSaveInitiations().empty() &&
              delayed.recorder().callbackOrder().empty() &&
              owner.federateSaveInitiations().empty() &&
              owner.recorder().callbackOrder().empty(),
          "Ready member callback service consumed delayed or regulator callbacks");

  waitFor(delayed,
          [&] {
            return delayed.recorder().timedInteractions().size() == 1U &&
                delayed.federateSaveInitiations().size() == 1U &&
                delayed.recorder().timeAdvanceGrants().size() == 1U;
          }, options, "delayed per-member TSO delivery and save initiation");
  auto const deliveries = delayed.recorder().timedInteractions();
  require(deliveries.size() == 1U &&
              deliveries.front().interaction == delayedInteraction &&
              deliveries.front().time == saveEncoded &&
              deliveries.front().parameters.size() == 1U &&
              copyBytes(deliveries.front().parameters.at(delayedParameter)) == payload &&
              deliveries.front().tag == tagBytes &&
              deliveries.front().receivedOrder == rti::TIMESTAMP,
          "Delayed member did not receive the timestamped interaction at save time");
  require(delayed.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              delayed.federateSaveTimes().size() == 1U &&
              delayed.federateSaveTimes().front().encoded == saveEncoded &&
              delayed.recorder().timeAdvanceGrants().front().encoded == saveEncoded &&
              delayed.recorder().callbackOrder() ==
                  std::vector<std::string>{"interaction", "federate-save-initiate", "grant"},
          "Delayed member did not receive TSO before save initiation and grant");
  require(owner.federateSaveInitiations().empty() &&
              owner.recorder().callbackOrder().empty(),
          "Regulator received save callbacks before its own service");

  waitFor(owner,
          [&] {
            return owner.federateSaveInitiations().size() == 1U &&
                owner.recorder().timeAdvanceGrants().size() == 1U;
          }, options, "per-member TSO regulator save initiation and grant");
  require(owner.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              owner.federateSaveTimes().size() == 1U &&
              owner.federateSaveTimes().front().encoded == saveEncoded &&
              owner.recorder().timeAdvanceGrants().front().encoded == saveEncoded &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "federate-save-initiate"},
          "Regulator did not grant and then initiate the timed save");

  for (auto* member : {&owner, &ready, &delayed}) {
    member->rtiAmbassador().federateSaveBegun();
  }
  for (auto* member : {&owner, &ready, &delayed}) {
    member->rtiAmbassador().federateSaveComplete();
  }
  waitForSessions(
      {&owner, &ready, &delayed},
      [&] {
        return owner.federationSavedCount() == 1U &&
            ready.federationSavedCount() == 1U &&
            delayed.federationSavedCount() == 1U;
      }, options, "three-member per-member TSO save completion");
  for (auto* member : {&owner, &ready, &delayed}) {
    require(member->federationNotSavedReasons().empty(),
            "Per-member TSO participant received Federation Not Saved");
  }

  delayed.rtiAmbassador().disableTimeConstrained();
  ready.rtiAmbassador().disableTimeConstrained();
  owner.rtiAmbassador().disableTimeRegulation();
  delayed.resign(rti::NO_ACTION);
  ready.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  delayed.disconnect();
  ready.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioTimedSavePerMemberTso(
    Options const& options, rti::CallbackModel model) {
  runTimedSavePerMemberTso(options, model);
}

void scenarioTimedSavePerMemberTsoContract(
    Options const& options, rti::CallbackModel model) {
  runTimedSavePerMemberTso(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
