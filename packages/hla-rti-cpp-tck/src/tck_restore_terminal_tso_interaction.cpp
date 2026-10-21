#include "tck_restore_terminal_tso_interaction.hpp"

#include <array>

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestoreTerminalTsoInteraction(Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Terminal timestamped interaction restore has native evidence only for HLA_EVOKED");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Terminal interaction restore requires adapter-supplied FOM and time implementation");
  Session sender(options, model, "owner");
  auto const federation = federationName(options, "restore-terminal-tso-interaction");
  std::wstring const saveLabel = L"tck-terminal-tso-retraction-baseline";
  std::array<std::uint8_t, 2> const payload{'T', 'M'};
  std::array<std::uint8_t, 3> const tagBytes{'T', 'O', 'M'};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  sender.connect();
  sender.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  sender.join(options.ownerFederateName, options.federateType, federation);
  auto const interaction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = sender.rtiAmbassador().getParameterHandle(
      interaction, options.parameterName);
  require(interaction.isValid() && parameter.isValid(),
          "Adapter FOM lacks terminal TSO interaction class or parameter");
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(parameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  sender.rtiAmbassador().publishInteractionClass(interaction);
  sender.rtiAmbassador().changeInteractionOrderType(interaction, rti::TIMESTAMP);
  auto const time = makeTimeContext(sender);
  auto const sendTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  sender.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitForSessions({&sender}, [&] {
    return sender.recorder().timeRegulationEnabled().size() == 1U;
  }, options, "terminal TSO interaction time-regulation enablement");

  auto const terminal = sender.rtiAmbassador().sendInteraction(
      interaction, parameters, tag, *sendTime);
  require(terminal.isValid(), "Terminal TSO interaction lacked a retraction handle");
  sender.rtiAmbassador().retract(terminal);
  requireException([&] { sender.rtiAmbassador().retract(terminal); },
                   L"MessageCanNoLongerBeRetracted",
                   "terminal TSO interaction retraction before save");

  sender.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions({&sender}, [&] {
    return sender.federateSaveInitiations() == std::vector<std::wstring>{saveLabel};
  }, options, "terminal TSO interaction save initiation");
  sender.rtiAmbassador().federateSaveBegun();
  sender.rtiAmbassador().federateSaveComplete();
  waitForSessions({&sender}, [&] {
    return sender.federationSavedCount() == 1U;
  }, options, "terminal TSO interaction federation saved");

  auto const discarded = sender.rtiAmbassador().sendInteraction(
      interaction, parameters, tag, *sendTime);
  require(discarded.isValid() &&
              copyBytes(terminal.encode()) != copyBytes(discarded.encode()),
          "Post-save interaction reused the saved terminal designator");
  sender.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions({&sender}, [&] {
    return sender.recorder().federationRestoreRequestsSucceeded().size() == 1U &&
           sender.federateRestoreInitiations().size() == 1U &&
           sender.federateRestoreInitiations().front().label == saveLabel;
  }, options, "terminal TSO interaction restore initiation");
  sender.rtiAmbassador().federateRestoreComplete();
  waitForSessions({&sender}, [&] {
    return sender.federationRestoredCount() == 1U;
  }, options, "terminal TSO interaction federation restored");
  requireException([&] { sender.rtiAmbassador().retract(terminal); },
                   L"MessageCanNoLongerBeRetracted",
                   "saved terminal TSO interaction classification");
  requireException([&] { sender.rtiAmbassador().retract(discarded); },
                   L"InvalidMessageRetractionHandle",
                   "discarded post-save TSO interaction designator");

  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  sender.disconnect();
}

}  // namespace

void scenarioRestoreTerminalTsoInteraction(Options const& options, rti::CallbackModel model) {
  runRestoreTerminalTsoInteraction(options, model);
}

void scenarioRestoreTerminalTsoInteractionContract(
    Options const& options, rti::CallbackModel model) {
  runRestoreTerminalTsoInteraction(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
