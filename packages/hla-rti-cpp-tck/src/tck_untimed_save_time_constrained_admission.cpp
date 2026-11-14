#include "tck_untimed_save_time_constrained_admission.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runUntimedSaveTimeConstrainedAdmission(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Untimed save constrained admission requires evoked callbacks");
  require(!options.fom.empty(),
          "Untimed save constrained admission requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Untimed save constrained admission requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session first(options, model, "first");
  Session second(options, model, "second");
  auto const federation =
      federationName(options, "untimed-save-time-constrained-admission");
  std::wstring const saveLabel = L"tck-untimed-save-time-constrained-admission";
  owner.connect();
  first.connect();
  second.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  first.join(options.memberFederateName + L"-first", options.federateType,
             federation);
  second.join(options.memberFederateName + L"-second", options.federateType,
              federation);

  auto const ownerTime = makeTimeContext(owner);
  auto const firstTime = makeTimeContext(first);
  auto const secondTime = makeTimeContext(second);
  require(ownerTime.factory->getName() == firstTime.factory->getName() &&
              ownerTime.factory->getName() == secondTime.factory->getName(),
          "Untimed-save members selected different logical-time factories");
  auto const pendingGrant =
      timeAfter(*ownerTime.factory, *ownerTime.initial, *ownerTime.epsilon, 5U);

  first.rtiAmbassador().enableTimeConstrained();
  waitFor(first,
          [&] { return first.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "first constrained-role enablement");
  second.rtiAmbassador().enableTimeConstrained();
  waitFor(second,
          [&] { return second.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "second constrained-role enablement");
  owner.rtiAmbassador().enableTimeRegulation(*ownerTime.epsilon);
  waitFor(owner,
          [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
          options, "save owner regulator enablement");
  owner.recorder().clearCallbackOrder();
  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();

  owner.rtiAmbassador().requestFederationSave(saveLabel);
  static_cast<void>(first.rtiAmbassador().evokeCallback(0.0));
  static_cast<void>(second.rtiAmbassador().evokeCallback(0.0));
  require(first.federateSaveInitiations().empty() &&
              second.federateSaveInitiations().empty(),
          "A constrained member received Initiate Federate Save before entering Time Advancing");

  first.rtiAmbassador().timeAdvanceRequest(*pendingGrant);
  static_cast<void>(first.rtiAmbassador().evokeCallback(0.0));
  static_cast<void>(second.rtiAmbassador().evokeCallback(0.0));
  require(first.recorder().timeAdvanceGrants().empty() &&
              second.federateSaveInitiations().empty(),
          "A constrained member received an ineligible callback before the second member entered Time Advancing");
  second.rtiAmbassador().timeAdvanceRequest(*pendingGrant);
  waitForSessions(
      {&first, &second},
      [&] {
        return first.federateSaveInitiations().size() == 1U &&
            second.federateSaveInitiations().size() == 1U;
      },
      options, "constrained-member save admission after both enter Time Advancing");
  require(first.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              second.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              first.federateSaveTimes().empty() &&
              second.federateSaveTimes().empty() &&
              first.recorder().timeAdvanceGrants().empty() &&
              second.recorder().timeAdvanceGrants().empty() &&
              first.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate"} &&
              second.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate"} &&
              owner.federateSaveInitiations().empty(),
          "Constrained members did not receive the matching untimed save before their grants");

  waitFor(owner,
          [&] { return owner.federateSaveInitiations().size() == 1U; },
          options, "save owner initiation callback");
  require(owner.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              owner.federateSaveTimes().empty() &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate"},
          "Untimed save owner received an unexpected initiation callback");

  for (auto* member : {&owner, &first, &second}) {
    member->rtiAmbassador().federateSaveBegun();
  }
  for (auto* member : {&owner, &first, &second}) {
    member->rtiAmbassador().federateSaveComplete();
  }
  waitForSessions(
      {&owner, &first, &second},
      [&] {
        return owner.federationSavedCount() == 1U &&
            first.federationSavedCount() == 1U &&
            second.federationSavedCount() == 1U;
      },
      options, "three-member untimed-save completion");
  require(owner.federationNotSavedReasons().empty() &&
              first.federationNotSavedReasons().empty() &&
              second.federationNotSavedReasons().empty(),
          "Untimed constrained-admission save reported a failure");

  first.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  second.disconnect();
  first.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioUntimedSaveTimeConstrainedAdmission(
    Options const& options, rti::CallbackModel model) {
  runUntimedSaveTimeConstrainedAdmission(options, model);
}

void scenarioUntimedSaveTimeConstrainedAdmissionContract(
    Options const& options, rti::CallbackModel model) {
  runUntimedSaveTimeConstrainedAdmission(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
