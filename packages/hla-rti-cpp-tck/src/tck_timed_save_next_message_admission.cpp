#include "tck_timed_save_next_message_admission.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runTimedSaveNextMessageAdmission(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Next-message timed save requires evoked callbacks");
  require(!options.fom.empty(),
          "Next-message timed save requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Next-message timed save requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "timed-save-next-message-admission");
  std::wstring const inclusiveLabel = L"tck-next-message-save-inclusive";
  std::wstring const exclusiveLabel = L"tck-next-message-save-exclusive";
  owner.connect();
  receiver.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  receiver.join(options.memberFederateName + L"-receiver", options.federateType,
                federation);

  auto const time = makeTimeContext(owner);
  require(time.factory->getName() == makeTimeContext(receiver).factory->getName(),
          "Next-message members selected different logical-time factories");
  auto const inclusiveTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const ownerFirstTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  auto const exclusiveTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 7U);
  auto const laterTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 8U);
  auto const inclusiveEncoded = encodeTime(*inclusiveTime);
  auto const ownerFirstEncoded = encodeTime(*ownerFirstTime);
  auto const exclusiveEncoded = encodeTime(*exclusiveTime);
  auto const laterEncoded = encodeTime(*laterTime);

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(receiver,
          [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "next-message constrained-role enablement");
  owner.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(owner,
          [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
          options, "next-message regulator enablement");
  owner.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();

  owner.rtiAmbassador().requestFederationSave(inclusiveLabel, *inclusiveTime);
  receiver.rtiAmbassador().nextMessageRequest(*inclusiveTime);
  owner.rtiAmbassador().timeAdvanceRequest(*inclusiveTime);
  waitFor(owner,
          [&] { return owner.recorder().timeAdvanceGrants().size() == 1U; },
          options, "inclusive NMR regulator grant");
  require(owner.recorder().timeAdvanceGrants().front().encoded == inclusiveEncoded &&
              owner.federateSaveInitiations().empty() &&
              owner.recorder().callbackOrder() == std::vector<std::string>{"grant"},
          "Inclusive NMR regulator initiated save before recipient admission");
  waitFor(receiver,
          [&] {
            return receiver.federateSaveInitiations().size() == 1U &&
                receiver.recorder().timeAdvanceGrants().size() == 1U;
          }, options, "inclusive NMR save initiation and grant");
  require(receiver.federateSaveInitiations() ==
                  std::vector<std::wstring>{inclusiveLabel} &&
              receiver.federateSaveTimes().size() == 1U &&
              receiver.federateSaveTimes().front().encoded == inclusiveEncoded &&
              receiver.recorder().timeAdvanceGrants().front().encoded == inclusiveEncoded &&
              receiver.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "grant"} &&
              owner.recorder().callbackOrder() == std::vector<std::string>{"grant"},
          "Inclusive NMR did not initiate save before equal grant 5");
  waitFor(owner,
          [&] { return owner.federateSaveInitiations().size() == 1U; },
          options, "inclusive NMR regulator save initiation");
  require(owner.federateSaveInitiations() ==
                  std::vector<std::wstring>{inclusiveLabel} &&
              owner.federateSaveTimes().size() == 1U &&
              owner.federateSaveTimes().front().encoded == inclusiveEncoded &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "federate-save-initiate"},
          "Inclusive NMR regulator did not receive matching save initiation");

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
      }, options, "inclusive NMR save completion");
  require(owner.federationNotSavedReasons().empty() &&
              receiver.federationNotSavedReasons().empty(),
          "Inclusive NMR participant received Federation Not Saved");

  owner.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();
  owner.rtiAmbassador().requestFederationSave(exclusiveLabel, *exclusiveTime);
  receiver.rtiAmbassador().nextMessageRequestAvailable(*exclusiveTime);
  owner.rtiAmbassador().timeAdvanceRequest(*ownerFirstTime);
  waitFor(owner,
          [&] { return owner.recorder().timeAdvanceGrants().size() == 2U; },
          options, "exclusive NMRA regulator first grant");
  require(owner.recorder().timeAdvanceGrants().back().encoded == ownerFirstEncoded &&
              owner.federateSaveInitiations() ==
                  std::vector<std::wstring>{inclusiveLabel} &&
              owner.recorder().callbackOrder() == std::vector<std::string>{"grant"},
          "Exclusive NMRA regulator initiated replacement save too early");
  waitFor(receiver,
          [&] { return receiver.recorder().timeAdvanceGrants().size() == 2U; },
          options, "exclusive NMRA equal-boundary grant");
  require(receiver.recorder().timeAdvanceGrants().back().encoded == exclusiveEncoded &&
              receiver.federateSaveInitiations() ==
                  std::vector<std::wstring>{inclusiveLabel} &&
              receiver.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant"} &&
              owner.recorder().callbackOrder() == std::vector<std::string>{"grant"},
          "Equal NMRA grant incorrectly initiated the replacement save");

  receiver.rtiAmbassador().nextMessageRequestAvailable(*laterTime);
  owner.rtiAmbassador().timeAdvanceRequest(*exclusiveTime);
  waitFor(owner,
          [&] { return owner.recorder().timeAdvanceGrants().size() == 3U; },
          options, "exclusive NMRA regulator second grant");
  require(owner.recorder().timeAdvanceGrants().back().encoded == exclusiveEncoded &&
              owner.federateSaveInitiations() ==
                  std::vector<std::wstring>{inclusiveLabel} &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "grant"},
          "Regulator received replacement save before qualifying NMRA admission");
  waitFor(receiver,
          [&] {
            return receiver.federateSaveInitiations().size() == 2U &&
                receiver.recorder().timeAdvanceGrants().size() == 3U;
          }, options, "exclusive NMRA strict-later save initiation and grant");
  require(receiver.federateSaveInitiations() ==
                  std::vector<std::wstring>{inclusiveLabel, exclusiveLabel} &&
              receiver.federateSaveTimes().size() == 2U &&
              receiver.federateSaveTimes().back().encoded == exclusiveEncoded &&
              receiver.recorder().timeAdvanceGrants().back().encoded == laterEncoded &&
              receiver.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "federate-save-initiate", "grant"} &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "grant"},
          "Strict-later NMRA did not initiate replacement save before grant 8");
  waitFor(owner,
          [&] { return owner.federateSaveInitiations().size() == 2U; },
          options, "exclusive NMRA regulator save initiation");
  require(owner.federateSaveInitiations() ==
                  std::vector<std::wstring>{inclusiveLabel, exclusiveLabel} &&
              owner.federateSaveTimes().size() == 2U &&
              owner.federateSaveTimes().back().encoded == exclusiveEncoded &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "grant", "federate-save-initiate"},
          "Regulator did not receive matching replacement save initiation");

  for (auto* member : {&owner, &receiver}) {
    member->rtiAmbassador().federateSaveBegun();
  }
  for (auto* member : {&owner, &receiver}) {
    member->rtiAmbassador().federateSaveComplete();
  }
  waitForSessions(
      {&owner, &receiver},
      [&] {
        return owner.federationSavedCount() == 2U &&
            receiver.federationSavedCount() == 2U;
      }, options, "exclusive NMRA save completion");
  require(owner.federationNotSavedReasons().empty() &&
              receiver.federationNotSavedReasons().empty(),
          "Next-message participant received Federation Not Saved");

  receiver.rtiAmbassador().disableTimeConstrained();
  owner.rtiAmbassador().disableTimeRegulation();
  receiver.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioTimedSaveNextMessageAdmission(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveNextMessageAdmission(options, model);
}

void scenarioTimedSaveNextMessageAdmissionContract(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveNextMessageAdmission(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
