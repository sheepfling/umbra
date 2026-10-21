#include "tck_timed_save_available_next_message_member_admission.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runTimedSaveAvailableNextMessageMemberAdmission(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Available/next-message timed save requires evoked callbacks");
  require(!options.fom.empty(),
          "Available/next-message timed save requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Available/next-message timed save requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session available(options, model, "available");
  Session nextMessage(options, model, "next-message");
  auto const federation =
      federationName(options, "timed-save-available-next-message-member-admission");
  std::wstring const saveLabel = L"tck-available-next-message-save";
  owner.connect();
  available.connect();
  nextMessage.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  available.join(options.memberFederateName + L"-available",
                 options.federateType, federation);
  nextMessage.join(options.memberFederateName + L"-next-message",
                   options.federateType, federation);

  auto const time = makeTimeContext(owner);
  require(time.factory->getName() == makeTimeContext(available).factory->getName() &&
              time.factory->getName() == makeTimeContext(nextMessage).factory->getName(),
          "Available/next-message members selected different logical-time factories");
  auto const saveTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const laterTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  auto const saveEncoded = encodeTime(*saveTime);
  auto const laterEncoded = encodeTime(*laterTime);

  available.rtiAmbassador().enableTimeConstrained();
  waitFor(available,
          [&] { return available.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "available member constrained-role enablement");
  nextMessage.rtiAmbassador().enableTimeConstrained();
  waitFor(nextMessage,
          [&] { return nextMessage.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "next-message member constrained-role enablement");
  owner.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(owner,
          [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
          options, "available/next-message regulator enablement");
  owner.recorder().clearCallbackOrder();
  available.recorder().clearCallbackOrder();
  nextMessage.recorder().clearCallbackOrder();

  owner.rtiAmbassador().requestFederationSave(saveLabel, *saveTime);
  available.rtiAmbassador().timeAdvanceRequestAvailable(*laterTime);
  nextMessage.rtiAmbassador().nextMessageRequestAvailable(*laterTime);
  owner.rtiAmbassador().timeAdvanceRequest(*saveTime);
  waitFor(owner,
          [&] { return owner.recorder().timeAdvanceGrants().size() == 1U; },
          options, "available/next-message regulator grant");
  require(owner.recorder().timeAdvanceGrants().front().encoded == saveEncoded &&
              owner.federateSaveInitiations().empty() &&
              owner.recorder().callbackOrder() == std::vector<std::string>{"grant"},
          "Regulator initiated save before constrained-member admission");

  waitFor(available,
          [&] {
            return available.federateSaveInitiations().size() == 1U &&
                available.recorder().timeAdvanceGrants().size() == 1U;
          }, options, "available member save initiation and grant");
  require(available.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              available.federateSaveTimes().size() == 1U &&
              available.federateSaveTimes().front().encoded == saveEncoded &&
              available.recorder().timeAdvanceGrants().front().encoded == laterEncoded &&
              available.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "grant"} &&
              nextMessage.federateSaveInitiations().empty() &&
              nextMessage.recorder().callbackOrder().empty() &&
              owner.federateSaveInitiations().empty(),
          "Available member was not admitted independently before its grant");

  waitFor(nextMessage,
          [&] {
            return nextMessage.federateSaveInitiations().size() == 1U &&
                nextMessage.recorder().timeAdvanceGrants().size() == 1U;
          }, options, "next-message member save initiation and grant");
  require(nextMessage.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              nextMessage.federateSaveTimes().size() == 1U &&
              nextMessage.federateSaveTimes().front().encoded == saveEncoded &&
              nextMessage.recorder().timeAdvanceGrants().front().encoded == laterEncoded &&
              nextMessage.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "grant"} &&
              owner.federateSaveInitiations().empty(),
          "Next-message member was not admitted independently before its grant");

  waitFor(owner,
          [&] { return owner.federateSaveInitiations().size() == 1U; },
          options, "available/next-message regulator save initiation");
  require(owner.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              owner.federateSaveTimes().size() == 1U &&
              owner.federateSaveTimes().front().encoded == saveEncoded &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "federate-save-initiate"},
          "Regulator did not wait for both constrained members");

  for (auto* member : {&owner, &available, &nextMessage}) {
    member->rtiAmbassador().federateSaveBegun();
  }
  for (auto* member : {&owner, &available, &nextMessage}) {
    member->rtiAmbassador().federateSaveComplete();
  }
  waitForSessions(
      {&owner, &available, &nextMessage},
      [&] {
        return owner.federationSavedCount() == 1U &&
            available.federationSavedCount() == 1U &&
            nextMessage.federationSavedCount() == 1U;
      }, options, "available/next-message save completion");
  require(owner.federationNotSavedReasons().empty() &&
              available.federationNotSavedReasons().empty() &&
              nextMessage.federationNotSavedReasons().empty(),
          "Available/next-message participant received Federation Not Saved");

  available.rtiAmbassador().disableTimeConstrained();
  nextMessage.rtiAmbassador().disableTimeConstrained();
  owner.rtiAmbassador().disableTimeRegulation();
  nextMessage.resign(rti::NO_ACTION);
  available.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  nextMessage.disconnect();
  available.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioTimedSaveAvailableNextMessageMemberAdmission(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveAvailableNextMessageMemberAdmission(options, model);
}

void scenarioTimedSaveAvailableNextMessageMemberAdmissionContract(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveAvailableNextMessageMemberAdmission(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
