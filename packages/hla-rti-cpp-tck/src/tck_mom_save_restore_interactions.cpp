#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

void verifyMomSaveRestoreInteractions(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "MOM save/restore interaction coverage requires an application FOM, the standard MIM, and logical-time configuration");

  Session subject(options, model, "mom-save-restore-interaction-subject");
  Session peer(options, model, "mom-save-restore-interaction-peer");
  auto const federation = federationName(options, "mom-save-restore-interactions");
  subject.connect();
  peer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-mom-save-restore-interaction-subject",
      options.federateType,
      federation);
  peer.join(
      options.memberFederateName + L"-mom-save-restore-interaction-peer",
      options.federateType,
      federation);

  auto& subjectRti = subject.rtiAmbassador();
  auto& peerRti = peer.rtiAmbassador();
  std::wstring const interactionPrefix =
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.";
  auto const saveBegun = subjectRti.getInteractionClassHandle(
      interactionPrefix + L"HLAfederateSaveBegun");
  auto const saveComplete = subjectRti.getInteractionClassHandle(
      interactionPrefix + L"HLAfederateSaveComplete");
  auto const restoreComplete = subjectRti.getInteractionClassHandle(
      interactionPrefix + L"HLAfederateRestoreComplete");
  auto const saveSuccess = subjectRti.getParameterHandle(
      saveComplete, L"HLAsuccessIndicator");
  auto const restoreSuccess = subjectRti.getParameterHandle(
      restoreComplete, L"HLAsuccessIndicator");
  auto const applicationInteraction =
      subjectRti.getInteractionClassHandle(options.interactionClassName);
  auto const applicationParameter = subjectRti.getParameterHandle(
      applicationInteraction, options.parameterName);
  require(
      saveBegun.isValid() && saveComplete.isValid() &&
          restoreComplete.isValid() && saveSuccess.isValid() &&
          restoreSuccess.isValid() && applicationInteraction.isValid() &&
          applicationParameter.isValid(),
      "The standard MIM does not expose the save/restore service interactions and success parameters");

  auto const peerSaveBegun = peerRti.getInteractionClassHandle(
      interactionPrefix + L"HLAfederateSaveBegun");
  auto const peerSaveComplete = peerRti.getInteractionClassHandle(
      interactionPrefix + L"HLAfederateSaveComplete");
  auto const peerRestoreComplete = peerRti.getInteractionClassHandle(
      interactionPrefix + L"HLAfederateRestoreComplete");
  auto const peerSaveSuccess = peerRti.getParameterHandle(
      peerSaveComplete, L"HLAsuccessIndicator");
  auto const peerRestoreSuccess = peerRti.getParameterHandle(
      peerRestoreComplete, L"HLAsuccessIndicator");
  require(
      saveBegun == peerSaveBegun && saveComplete == peerSaveComplete &&
          restoreComplete == peerRestoreComplete &&
          saveSuccess == peerSaveSuccess && restoreSuccess == peerRestoreSuccess,
      "Standard MIM interaction handles differed across joined federates");

  for (auto const& interaction : {saveBegun, saveComplete, restoreComplete}) {
    subjectRti.publishInteractionClass(interaction);
    peerRti.publishInteractionClass(interaction);
  }
  subjectRti.publishInteractionClass(applicationInteraction);
  peerRti.publishInteractionClass(applicationInteraction);
  std::vector<std::uint8_t> applicationPayload{0x53U, 0x52U};
  rti::ParameterHandleValueMap applicationParameters;
  applicationParameters.emplace(
      applicationParameter,
      rti::VariableLengthData(
          applicationPayload.data(), applicationPayload.size()));

  auto const saveLabel = std::wstring{L"mom-save-restore-interactions"};
  subjectRti.requestFederationSave(saveLabel);
  waitForSessions(
      {&subject, &peer},
      [&] {
        return subject.federateSaveInitiations() ==
                std::vector<std::wstring>{saveLabel} &&
            peer.federateSaveInitiations() == std::vector<std::wstring>{saveLabel};
      },
      options,
      "MOM save interaction save-initiation callbacks");
  requireException(
      [&] {
        subjectRti.sendInteraction(
            applicationInteraction,
            applicationParameters,
            rti::VariableLengthData{});
      },
      L"SaveInProgress",
      "ordinary application interaction remains unavailable during save");

  subjectRti.sendInteraction(
      saveBegun,
      rti::ParameterHandleValueMap{},
      rti::VariableLengthData{});
  peerRti.sendInteraction(
      peerSaveBegun,
      rti::ParameterHandleValueMap{},
      rti::VariableLengthData{});

  auto saveCompleteParameters = [&](rti::ParameterHandle const& parameter) {
    rti::HLAboolean success{true};
    rti::ParameterHandleValueMap parameters;
    parameters.emplace(parameter, success.encode());
    return parameters;
  };
  subjectRti.sendInteraction(
      saveComplete,
      saveCompleteParameters(saveSuccess),
      rti::VariableLengthData{});
  peerRti.sendInteraction(
      peerSaveComplete,
      saveCompleteParameters(peerSaveSuccess),
      rti::VariableLengthData{});
  waitForSessions(
      {&subject, &peer},
      [&] {
        return subject.federationSavedCount() == 1U &&
            peer.federationSavedCount() == 1U;
      },
      options,
      "MOM save interactions completing the federation save");

  subjectRti.requestFederationRestore(saveLabel);
  waitForSessions(
      {&subject, &peer},
      [&] {
        return subject.federateRestoreInitiations().size() == 1U &&
            peer.federateRestoreInitiations().size() == 1U &&
            subject.federateRestoreInitiations().front().label == saveLabel &&
            peer.federateRestoreInitiations().front().label == saveLabel;
      },
      options,
      "MOM restore interaction restore-initiation callbacks");
  requireException(
      [&] {
        subjectRti.sendInteraction(
            applicationInteraction,
            applicationParameters,
            rti::VariableLengthData{});
      },
      L"RestoreInProgress",
      "ordinary application interaction remains unavailable during restore");

  auto restoreCompleteParameters = [&](rti::ParameterHandle const& parameter) {
    rti::HLAboolean success{true};
    rti::ParameterHandleValueMap parameters;
    parameters.emplace(parameter, success.encode());
    return parameters;
  };
  subjectRti.sendInteraction(
      restoreComplete,
      restoreCompleteParameters(restoreSuccess),
      rti::VariableLengthData{});
  peerRti.sendInteraction(
      peerRestoreComplete,
      restoreCompleteParameters(peerRestoreSuccess),
      rti::VariableLengthData{});
  waitForSessions(
      {&subject, &peer},
      [&] {
        return subject.federationRestoredCount() == 1U &&
            peer.federationRestoredCount() == 1U;
      },
      options,
      "MOM restore interactions completing the federation restore");

  peer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subjectRti.destroyFederationExecution(federation);
  peer.disconnect();
  subject.disconnect();
}

}  // namespace

void scenarioMomSaveRestoreInteractions(
    Options const& options,
    rti::CallbackModel model) {
  verifyMomSaveRestoreInteractions(options, model);
}

void scenarioMomSaveRestoreInteractionsContract(
    Options const& options,
    rti::CallbackModel model) {
  verifyMomSaveRestoreInteractions(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
