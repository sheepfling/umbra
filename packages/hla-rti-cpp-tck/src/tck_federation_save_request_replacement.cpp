#include "tck_federation_save_request_replacement.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runFederationSaveRequestReplacement(
    Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Save-request replacement requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Save-request replacement requires a logical-time implementation");

  Session owner(options, model, "save-request-owner");
  Session constrained(options, model, "save-request-constrained");
  auto const federation = federationName(options, "save-request-replacement");
  std::wstring const supersededLabel = L"tck-save-request-superseded";
  std::wstring const replacementLabel = L"tck-save-request-replacement";

  owner.connect();
  constrained.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  constrained.join(
      options.memberFederateName, options.federateType, federation);

  auto const time = makeTimeContext(owner);
  auto const constrainedTime = makeTimeContext(constrained);
  require(time.factory->getName() == constrainedTime.factory->getName(),
          "Save-request members selected different logical-time factories");
  auto const supersededTime =
      timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const replacementTime =
      timeAfter(*time.factory, *time.initial, *time.epsilon, 7U);
  auto const supersededEncoded = encodeTime(*supersededTime);
  auto const replacementEncoded = encodeTime(*replacementTime);

  constrained.rtiAmbassador().enableTimeConstrained();
  waitFor(
      constrained,
      [&] { return constrained.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "save-request constrained-role enablement");
  owner.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(
      owner,
      [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "save-request regulator enablement");

  owner.rtiAmbassador().requestFederationSave(
      supersededLabel, *supersededTime);
  owner.rtiAmbassador().requestFederationSave(
      replacementLabel, *replacementTime);

  constrained.rtiAmbassador().timeAdvanceRequest(*supersededTime);
  owner.rtiAmbassador().timeAdvanceRequest(*supersededTime);
  waitForSessions(
      {&owner, &constrained},
      [&] {
        return owner.recorder().timeAdvanceGrants().size() == 1U &&
            constrained.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "first time advance before the replacement save timestamp");
  require(
      owner.recorder().timeAdvanceGrants().front().encoded == supersededEncoded &&
          constrained.recorder().timeAdvanceGrants().front().encoded ==
              supersededEncoded &&
          owner.federateSaveInitiations().empty() &&
          constrained.federateSaveInitiations().empty(),
      "superseded save request initiated at its former timestamp");

  constrained.rtiAmbassador().timeAdvanceRequest(*replacementTime);
  owner.rtiAmbassador().timeAdvanceRequest(*replacementTime);
  waitForSessions(
      {&owner, &constrained},
      [&] {
        return owner.recorder().timeAdvanceGrants().size() == 2U &&
            constrained.recorder().timeAdvanceGrants().size() == 2U &&
            owner.federateSaveInitiations().size() == 1U &&
            constrained.federateSaveInitiations().size() == 1U;
      },
      options,
      "replacement save initiation at its requested timestamp");

  auto const ownerSaveTimes = owner.federateSaveTimes();
  auto const constrainedSaveTimes = constrained.federateSaveTimes();
  require(
      owner.federateSaveInitiations() ==
              std::vector<std::wstring>{replacementLabel} &&
          constrained.federateSaveInitiations() ==
              std::vector<std::wstring>{replacementLabel} &&
          owner.recorder().timeAdvanceGrants().back().encoded ==
              replacementEncoded &&
          constrained.recorder().timeAdvanceGrants().back().encoded ==
              replacementEncoded &&
          ownerSaveTimes.size() == 1U &&
          ownerSaveTimes.front().encoded == replacementEncoded &&
          constrainedSaveTimes.size() == 1U &&
          constrainedSaveTimes.front().encoded == replacementEncoded &&
          owner.federationSavedCount() == 0U &&
          constrained.federationSavedCount() == 0U,
      "new save request did not replace the outstanding request with its label and time");

  owner.rtiAmbassador().federateSaveBegun();
  constrained.rtiAmbassador().federateSaveBegun();
  owner.rtiAmbassador().federateSaveComplete();
  constrained.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &constrained},
      [&] {
        return owner.federationSavedCount() == 1U &&
            constrained.federationSavedCount() == 1U;
      },
      options,
      "replacement save completion");
  require(owner.federationNotSavedReasons().empty() &&
              constrained.federationNotSavedReasons().empty(),
          "replacement save failed instead of completing");

  constrained.rtiAmbassador().disableTimeConstrained();
  owner.rtiAmbassador().disableTimeRegulation();
  constrained.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  constrained.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioFederationSaveRequestReplacement(
    Options const& options, rti::CallbackModel model) {
  runFederationSaveRequestReplacement(options, model);
}

void scenarioFederationSaveRequestReplacementContract(
    Options const& options, rti::CallbackModel model) {
  runFederationSaveRequestReplacement(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
