#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioConnectionDefaultConfigurationFallback(
    Options const& options, rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "default-configuration fallback coverage requires an adapter-supplied FOM module");

  auto defaultOptions = options;
  defaultOptions.configurationName.clear();
  defaultOptions.ownerConfigurationName.clear();
  defaultOptions.memberConfigurationName.clear();
  defaultOptions.rtiAddress.clear();
  defaultOptions.ownerRtiAddress.clear();
  defaultOptions.memberRtiAddress.clear();
  defaultOptions.additionalSettings.clear();

  auto const nonce = std::chrono::steady_clock::now().time_since_epoch().count();
  auto const federation = toWide(
      options.federationPrefix + "-default-configuration-fallback-" +
      std::to_string(nonce));

  Session defaultConnection(defaultOptions, model, "owner");
  defaultConnection.connect();
  bool federationCreated = false;
  try {
    defaultConnection.rtiAmbassador().createFederationExecution(
        federation,
        options.fom.wstring(),
        options.logicalTimeImplementationName);
    federationCreated = true;

    Session unknownNameConnection(defaultOptions, model, "member");
    auto const unknownName =
        std::wstring(L"hla-rti-tck-unknown-configuration-") +
        std::to_wstring(nonce);
    auto configuration = rti::RtiConfiguration::createConfiguration();
    configuration.withConfigurationName(unknownName);
    unknownNameConnection.connectWithConfiguration(configuration);

    unknownNameConnection.rtiAmbassador().listFederationExecutions();
    waitFor(
        unknownNameConnection,
        [&] { return unknownNameConnection.recorder().listsFederation(federation); },
        defaultOptions,
        "a federation created through the no-name implementation default to appear after unknown-name fallback");
    unknownNameConnection.disconnect();

    defaultConnection.rtiAmbassador().destroyFederationExecution(federation);
    federationCreated = false;
  } catch (...) {
    if (federationCreated) {
      try {
        defaultConnection.rtiAmbassador().destroyFederationExecution(federation);
      } catch (...) {
      }
    }
    throw;
  }
  defaultConnection.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
