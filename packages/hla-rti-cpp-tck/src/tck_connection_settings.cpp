#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioConnectionAdditionalSettingsParseFailure(
    Options const& options, rti::CallbackModel model) {
  require(
      !options.additionalSettings.empty(),
      "connection settings parse-failure coverage requires an adapter-supplied malformed setting");

  rti::RTIambassadorFactory factory;
  auto ambassador = factory.createRTIambassador();
  require(
      static_cast<bool>(ambassador),
      "settings parse-failure check did not create a standard RTIambassador");

  Recorder recorder;
  auto configuration = rti::RtiConfiguration::createConfiguration();
  configuration.withAdditionalSettings(toWide(options.additionalSettings));
  auto const result = ambassador->connect(recorder, model, configuration);
  require(
      result.additionalSettingsResult == rti::SETTINGS_FAILED_TO_PARSE,
      "Connect did not report SETTINGS_FAILED_TO_PARSE for the adapter-supplied malformed setting");
  ambassador->disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
