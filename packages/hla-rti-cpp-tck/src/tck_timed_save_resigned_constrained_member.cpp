#include "tck_timed_save_resigned_constrained_member.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runTimedSaveResignedConstrainedMember(
    Options const& options, rti::CallbackModel model, bool pendingFqr) {
  require(model == rti::HLA_EVOKED,
          "Resigned constrained-member timed save requires evoked callbacks");
  require(!options.fom.empty(),
          "Resigned constrained-member timed save requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Resigned constrained-member timed save requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session ordinary(options, model, "ordinary");
  Session departing(options, model, "departing");
  auto const federation =
      federationName(options, "timed-save-resigned-constrained-member");
  std::wstring const saveLabel = L"tck-resigned-constrained-save";
  owner.connect();
  ordinary.connect();
  departing.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  ordinary.join(options.memberFederateName + L"-ordinary",
                options.federateType, federation);
  departing.join(options.memberFederateName + L"-departing",
                 options.federateType, federation);

  auto const time = makeTimeContext(owner);
  require(time.factory->getName() == makeTimeContext(ordinary).factory->getName() &&
              time.factory->getName() == makeTimeContext(departing).factory->getName(),
          "Timed-save members selected different logical-time factories");
  auto const saveTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const fqrTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  auto const saveEncoded = encodeTime(*saveTime);

  ordinary.rtiAmbassador().enableTimeConstrained();
  waitFor(ordinary,
          [&] { return ordinary.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "remaining member constrained-role enablement");
  departing.rtiAmbassador().enableTimeConstrained();
  waitFor(departing,
          [&] { return departing.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "departing member constrained-role enablement");
  owner.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(owner,
          [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
          options, "regulator enablement");
  owner.recorder().clearCallbackOrder();
  ordinary.recorder().clearCallbackOrder();
  departing.recorder().clearCallbackOrder();

  owner.rtiAmbassador().requestFederationSave(saveLabel, *saveTime);
  if (pendingFqr) {
    departing.rtiAmbassador().flushQueueRequest(*fqrTime);
  }
  departing.resign(rti::NO_ACTION);
  ordinary.rtiAmbassador().timeAdvanceRequest(*saveTime);
  owner.rtiAmbassador().timeAdvanceRequest(*saveTime);
  waitFor(ordinary,
          [&] {
            return ordinary.federateSaveInitiations().size() == 1U &&
                ordinary.recorder().timeAdvanceGrants().size() == 1U;
          }, options, "remaining member save initiation and grant");
  require(ordinary.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              ordinary.federateSaveTimes().size() == 1U &&
              ordinary.federateSaveTimes().front().encoded == saveEncoded &&
              ordinary.recorder().timeAdvanceGrants().front().encoded == saveEncoded &&
              ordinary.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "grant"} &&
              owner.federateSaveInitiations().empty() &&
              departing.federateSaveInitiations().empty() &&
              departing.recorder().callbackOrder().empty(),
          "Resigned member still gated or received the pending save");

  waitFor(owner,
          [&] { return owner.federateSaveInitiations().size() == 1U; },
          options, "regulator save initiation after member resignation");
  require(owner.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              owner.federateSaveTimes().size() == 1U &&
              owner.federateSaveTimes().front().encoded == saveEncoded &&
              departing.federateSaveInitiations().empty(),
          "Regulator did not receive the pending save after member resignation");

  owner.rtiAmbassador().federateSaveBegun();
  ordinary.rtiAmbassador().federateSaveBegun();
  owner.rtiAmbassador().federateSaveComplete();
  ordinary.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &ordinary},
      [&] {
        return owner.federationSavedCount() == 1U &&
            ordinary.federationSavedCount() == 1U;
      }, options, "surviving-member save completion");
  require(owner.federationNotSavedReasons().empty() &&
              ordinary.federationNotSavedReasons().empty() &&
              departing.federationSavedCount() == 0U &&
              departing.federationNotSavedReasons().empty(),
          "Departed member received a save result or surviving save failed");

  ordinary.rtiAmbassador().disableTimeConstrained();
  owner.rtiAmbassador().disableTimeRegulation();
  ordinary.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  departing.disconnect();
  ordinary.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioTimedSaveResignedConstrainedMember(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveResignedConstrainedMember(options, model, false);
}

void scenarioTimedSaveResignedConstrainedMemberContract(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveResignedConstrainedMember(options, model, false);
}

void scenarioTimedSaveResignedPendingFqr(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveResignedConstrainedMember(options, model, true);
}

void scenarioTimedSaveResignedPendingFqrContract(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveResignedConstrainedMember(options, model, true);
}

}  // namespace hla_rti_cpp_tck::detail
