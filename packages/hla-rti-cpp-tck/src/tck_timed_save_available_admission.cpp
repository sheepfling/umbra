#include "tck_timed_save_available_admission.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runTimedSaveAvailableAdmission(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Available-advance timed save requires evoked callbacks");
  require(!options.fom.empty(),
          "Available-advance timed save requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Available-advance timed save requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "timed-save-available-admission");
  std::wstring const saveLabel = L"tck-timed-save-available-admission";
  owner.connect();
  receiver.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  receiver.join(options.memberFederateName + L"-receiver", options.federateType,
                federation);

  auto const time = makeTimeContext(owner);
  require(time.factory->getName() == makeTimeContext(receiver).factory->getName(),
          "Available-advance members selected different logical-time factories");
  auto const firstOwnerTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 4U);
  auto const saveTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const laterTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  auto const firstOwnerEncoded = encodeTime(*firstOwnerTime);
  auto const saveEncoded = encodeTime(*saveTime);
  auto const laterEncoded = encodeTime(*laterTime);

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(receiver,
          [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "available-advance constrained-role enablement");
  owner.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(owner,
          [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
          options, "available-advance regulator enablement");
  owner.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();

  owner.rtiAmbassador().requestFederationSave(saveLabel, *saveTime);
  receiver.rtiAmbassador().timeAdvanceRequestAvailable(*saveTime);
  owner.rtiAmbassador().timeAdvanceRequest(*firstOwnerTime);
  waitFor(owner,
          [&] { return owner.recorder().timeAdvanceGrants().size() == 1U; },
          options, "available-advance regulator first grant");
  require(owner.recorder().timeAdvanceGrants().front().encoded == firstOwnerEncoded &&
              owner.federateSaveInitiations().empty() &&
              owner.recorder().callbackOrder() == std::vector<std::string>{"grant"},
          "Regulator's first grant incorrectly initiated the timed save");
  waitFor(receiver,
          [&] { return receiver.recorder().timeAdvanceGrants().size() == 1U; },
          options, "available-advance equal-boundary grant");
  require(receiver.recorder().timeAdvanceGrants().front().encoded == saveEncoded &&
              receiver.federateSaveInitiations().empty() &&
              receiver.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant"} &&
              owner.recorder().callbackOrder() == std::vector<std::string>{"grant"},
          "Equal available-advance grant incorrectly initiated the save");

  receiver.rtiAmbassador().timeAdvanceRequestAvailable(*laterTime);
  owner.rtiAmbassador().timeAdvanceRequest(*saveTime);
  waitFor(owner,
          [&] { return owner.recorder().timeAdvanceGrants().size() == 2U; },
          options, "available-advance regulator second grant");
  require(owner.recorder().timeAdvanceGrants().back().encoded == saveEncoded &&
              owner.federateSaveInitiations().empty() &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "grant"},
          "Regulator received save initiation before qualifying recipient admission");
  waitFor(receiver,
          [&] {
            return receiver.federateSaveInitiations().size() == 1U &&
                receiver.recorder().timeAdvanceGrants().size() == 2U;
          }, options, "available-advance strict-later save initiation and grant");
  require(receiver.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              receiver.federateSaveTimes().size() == 1U &&
              receiver.federateSaveTimes().front().encoded == saveEncoded &&
              receiver.recorder().timeAdvanceGrants().back().encoded == laterEncoded &&
              receiver.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "federate-save-initiate", "grant"} &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "grant"},
          "Strict-later available-advance grant did not initiate save first");

  waitFor(owner,
          [&] { return owner.federateSaveInitiations().size() == 1U; },
          options, "available-advance regulator save initiation");
  require(owner.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              owner.federateSaveTimes().size() == 1U &&
              owner.federateSaveTimes().front().encoded == saveEncoded &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "grant", "federate-save-initiate"},
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
      }, options, "two-member available-advance save completion");
  require(owner.federationNotSavedReasons().empty() &&
              receiver.federationNotSavedReasons().empty(),
          "Available-advance participant received Federation Not Saved");

  receiver.rtiAmbassador().disableTimeConstrained();
  owner.rtiAmbassador().disableTimeRegulation();
  receiver.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioTimedSaveAvailableAdmission(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveAvailableAdmission(options, model);
}

void scenarioTimedSaveAvailableAdmissionContract(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveAvailableAdmission(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
