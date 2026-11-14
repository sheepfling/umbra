#include "tck_federate_save_timestamp_eligibility.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runFederateSaveTimestampEligibility(
    Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Federation-save timestamp eligibility requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Federation-save timestamp eligibility requires a logical-time implementation");

  Session federate(options, model, "save-timestamp-eligibility");
  auto const federation =
      federationName(options, "federation-save-timestamp-eligibility");
  federate.connect();
  federate.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  federate.join(options.ownerFederateName, options.federateType, federation);

  auto time = makeTimeContext(federate);
  auto galt = time.factory->makeInitial();
  require(galt != nullptr,
          "Logical-time factory returned no output value for the GALT query");
  require(!federate.rtiAmbassador().queryGALT(*galt),
          "GALT was defined before any federate enabled time regulation");

  auto const rejectedTime = timeAfter(
      *time.factory, *time.initial, *time.epsilon, 3U);
  std::wstring const rejectedLabel = L"tck-save-time-ineligible";
  requireException(
      [&] {
        federate.rtiAmbassador().requestFederationSave(
            rejectedLabel, *rejectedTime);
      },
      L"FederateUnableToUseTime",
      "timestamped Federation Save by a non-regulating federate with undefined GALT");

  auto const quietDeadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(150);
  while (std::chrono::steady_clock::now() < quietDeadline) {
    federate.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(federate.federateSaveInitiations().empty() &&
              federate.federateSaveTimes().empty() &&
              federate.federationSavedCount() == 0U,
          "rejected timestamped Federation Save left a save pending or emitted callbacks");

  federate.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(
      federate,
      [&] { return federate.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "time-regulation enablement for the eligible save request");

  auto const saveTime = timeAfter(
      *time.factory, *time.initial, *time.epsilon, 3U);
  std::wstring const saveLabel = L"tck-save-time-eligible-regulator";
  federate.rtiAmbassador().requestFederationSave(saveLabel, *saveTime);
  waitFor(
      federate,
      [&] { return federate.federateSaveInitiations().size() == 1U; },
      options,
      "timestamped save initiation for a time-regulating federate");
  auto const saveTimes = federate.federateSaveTimes();
  require(federate.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              saveTimes.size() == 1U &&
              saveTimes.front().encoded == encodeTime(*saveTime),
          "eligible timestamped Federation Save returned the wrong label or time");

  federate.rtiAmbassador().federateSaveBegun();
  federate.rtiAmbassador().federateSaveComplete();
  waitFor(
      federate,
      [&] { return federate.federationSavedCount() == 1U; },
      options,
      "timestamped save completion for a time-regulating federate");
  require(federate.federationNotSavedReasons().empty(),
          "eligible timestamped Federation Save reported a failure");

  federate.resign(rti::NO_ACTION);
  federate.rtiAmbassador().destroyFederationExecution(federation);
  federate.disconnect();
}

}  // namespace

void scenarioFederateSaveTimestampEligibility(
    Options const& options, rti::CallbackModel model) {
  runFederateSaveTimestampEligibility(options, model);
}

void scenarioFederateSaveTimestampEligibilityContract(
    Options const& options, rti::CallbackModel model) {
  runFederateSaveTimestampEligibility(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
