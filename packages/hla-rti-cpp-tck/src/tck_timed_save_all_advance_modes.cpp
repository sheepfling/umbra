#include "tck_timed_save_all_advance_modes.hpp"

#include <array>

namespace hla_rti_cpp_tck::detail {
namespace {

void runTimedSaveAllAdvanceModes(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "All-advance-mode timed save requires evoked callbacks");
  require(!options.fom.empty(),
          "All-advance-mode timed save requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "All-advance-mode timed save requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session tar(options, model, "member");
  Session nmr(options, model, "member");
  Session tara(options, model, "member");
  Session nmra(options, model, "member");
  Session fqr(options, model, "member");
  std::array<Session*, 6> const members{&owner, &tar, &nmr, &tara, &nmra, &fqr};
  std::array<Session*, 5> const constrained{&tar, &nmr, &tara, &nmra, &fqr};
  auto const federation = federationName(options, "timed-save-all-advance-modes");
  std::wstring const saveLabel = L"tck-timed-save-all-advance-modes";

  for (auto* member : members) {
    member->connect();
  }
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  tar.join(options.memberFederateName + L"-tar", options.federateType, federation);
  nmr.join(options.memberFederateName + L"-nmr", options.federateType, federation);
  tara.join(options.memberFederateName + L"-tara", options.federateType, federation);
  nmra.join(options.memberFederateName + L"-nmra", options.federateType, federation);
  fqr.join(options.memberFederateName + L"-fqr", options.federateType, federation);

  auto const time = makeTimeContext(owner);
  for (auto* member : constrained) {
    require(time.factory->getName() == makeTimeContext(*member).factory->getName(),
            "All-advance-mode members selected different logical-time factories");
  }
  auto const saveTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const laterTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  auto const saveEncoded = encodeTime(*saveTime);
  auto const laterEncoded = encodeTime(*laterTime);

  for (auto* member : constrained) {
    member->rtiAmbassador().enableTimeConstrained();
    waitFor(*member,
            [&] { return member->recorder().timeConstrainedEnabled().size() == 1U; },
            options, "all-advance-mode constrained-role enablement");
    member->recorder().clearCallbackOrder();
  }
  owner.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(owner,
          [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
          options, "all-advance-mode regulator enablement");
  owner.recorder().clearCallbackOrder();

  owner.rtiAmbassador().requestFederationSave(saveLabel, *saveTime);
  tar.rtiAmbassador().timeAdvanceRequest(*saveTime);
  nmr.rtiAmbassador().nextMessageRequest(*saveTime);
  tara.rtiAmbassador().timeAdvanceRequestAvailable(*laterTime);
  nmra.rtiAmbassador().nextMessageRequestAvailable(*laterTime);
  fqr.rtiAmbassador().flushQueueRequest(*laterTime);
  owner.rtiAmbassador().timeAdvanceRequest(*saveTime);

  auto verifyConstrained = [&](Session& member, bool flushGrant,
                               std::vector<std::uint8_t> const& grantEncoded) {
    waitFor(member,
            [&] {
              return member.federateSaveInitiations().size() == 1U &&
                  (flushGrant ? member.recorder().flushQueueGrants().size() == 1U
                              : member.recorder().timeAdvanceGrants().size() == 1U);
            }, options, "all-advance-mode save initiation and grant");
    require(member.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
                member.federateSaveTimes().size() == 1U &&
                member.federateSaveTimes().front().encoded == saveEncoded,
            "Constrained member did not receive the timed save initiation");
    if (flushGrant) {
      require(member.recorder().flushQueueGrants().front().time == grantEncoded &&
                  member.recorder().callbackOrder() ==
                      std::vector<std::string>{"federate-save-initiate", "flush-grant"},
              "FQR member did not initiate save before actual flush grant 6");
    } else {
      require(member.recorder().timeAdvanceGrants().front().encoded == grantEncoded &&
                  member.recorder().callbackOrder() ==
                      std::vector<std::string>{"federate-save-initiate", "grant"},
              "Constrained advance-mode member did not initiate save before grant");
    }
  };
  auto requireUnserviced = [&](std::size_t first) {
    for (std::size_t index = first; index < constrained.size(); ++index) {
      require(constrained[index]->federateSaveInitiations().empty() &&
                  constrained[index]->recorder().callbackOrder().empty(),
              "Later constrained member received a callback before its own service");
    }
    require(owner.federateSaveInitiations().empty() &&
                owner.recorder().callbackOrder().empty(),
            "Regulator received a callback before its own service");
  };

  verifyConstrained(tar, false, saveEncoded);
  requireUnserviced(1U);
  verifyConstrained(nmr, false, saveEncoded);
  requireUnserviced(2U);
  verifyConstrained(tara, false, laterEncoded);
  requireUnserviced(3U);
  verifyConstrained(nmra, false, laterEncoded);
  requireUnserviced(4U);
  verifyConstrained(fqr, true, laterEncoded);
  requireUnserviced(5U);

  waitFor(owner,
          [&] {
            return owner.federateSaveInitiations().size() == 1U &&
                owner.recorder().timeAdvanceGrants().size() == 1U;
          }, options, "all-advance-mode regulator save initiation and grant");
  require(owner.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              owner.federateSaveTimes().size() == 1U &&
              owner.federateSaveTimes().front().encoded == saveEncoded &&
              owner.recorder().timeAdvanceGrants().front().encoded == saveEncoded &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "federate-save-initiate"},
          "Regulator did not grant and then initiate the matching timed save");

  for (auto* member : members) {
    member->rtiAmbassador().federateSaveBegun();
  }
  for (auto* member : members) {
    member->rtiAmbassador().federateSaveComplete();
  }
  waitForSessions(
      {&owner, &tar, &nmr, &tara, &nmra, &fqr},
      [&] {
        for (auto* member : members) {
          if (member->federationSavedCount() != 1U) return false;
        }
        return true;
      }, options, "six-member all-advance-mode save completion");
  for (auto* member : members) {
    require(member->federationNotSavedReasons().empty(),
            "All-advance-mode member received Federation Not Saved");
  }

  for (auto* member : constrained) {
    member->rtiAmbassador().disableTimeConstrained();
  }
  owner.rtiAmbassador().disableTimeRegulation();
  for (auto index = members.size(); index > 0U; --index) {
    members[index - 1U]->resign(rti::NO_ACTION);
  }
  owner.rtiAmbassador().destroyFederationExecution(federation);
  for (auto index = members.size(); index > 0U; --index) {
    members[index - 1U]->disconnect();
  }
}

}  // namespace

void scenarioTimedSaveAllAdvanceModes(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveAllAdvanceModes(options, model);
}

void scenarioTimedSaveAllAdvanceModesContract(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveAllAdvanceModes(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
