#include "internal/federation/process_federation_service_runtime.hpp"

#include <cstdio>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_host.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/transport_protocol.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "internal/time/reference_time_selection.hpp"

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

std::filesystem::path processFederationResourceDirectory() {
  auto hasRequiredResources = [](std::filesystem::path const& root) {
    std::error_code error;
    auto isRegularFile = [&error](std::filesystem::path const& path) {
      error.clear();
      return std::filesystem::is_regular_file(path, error) && !error;
    };
    return isRegularFile(root / "schemas" / "IEEE1516-FDD-2025.xsd") &&
        isRegularFile(root / "schemas" / "IEEE1516-DIF-2025.xsd") &&
        isRegularFile(root / "mim" / "HLAstandardMIM-2025.xml");
  };

#ifdef UMBRA_EMBEDDED_FOM_RESOURCE_DIRECTORY
  auto const sourceTree =
      std::filesystem::path(UMBRA_EMBEDDED_FOM_RESOURCE_DIRECTORY);
  if (hasRequiredResources(sourceTree)) {
    return sourceTree;
  }
#endif

#ifdef UMBRA_EMBEDDED_FOM_RESOURCE_DIRECTORY_INSTALL
  auto const installedTree =
      std::filesystem::path(UMBRA_EMBEDDED_FOM_RESOURCE_DIRECTORY_INSTALL);
  if (hasRequiredResources(installedTree)) {
    return installedTree;
  }
#endif

  throw std::runtime_error(
      "Umbra could not locate the IEEE 1516.2-2025 schema/MIM resources for the process coordinator.");
}

struct ProcessCoordinatorFomContext final {
  explicit ProcessCoordinatorFomContext(std::filesystem::path const& resourceRoot)
      : composer(resourceRoot / "schemas" / "IEEE1516-FDD-2025.xsd"),
        coordinator(
            validator,
            composer,
            timeSelector,
            {
                resourceRoot / "mim" / "HLAstandardMIM-2025.xml",
                resourceRoot / "schemas" / "IEEE1516-DIF-2025.xsd",
                std::nullopt,
            }) {}

  umbra::detail::LibXml2FomValidator validator;
  umbra::detail::LibXml2FomModuleComposer composer;
  umbra::detail::ReferenceLogicalTimeSelector timeSelector;
  umbra::detail::FederationManagementCoordinator coordinator;
};

umbra::detail::ProcessFederationServiceOptions makeProcessServiceOptions(
    ProcessCoordinatorFomContext& fomContext,
    std::filesystem::path reportDirectory) {
  umbra::detail::ProcessFederationServiceOptions options;
  options.pushReceiveOrderEvents = true;
  options.serviceReportDirectory = std::move(reportDirectory);
  options.createFomPreparation = [&fomContext](
      std::vector<std::wstring> const& fomModules,
      std::optional<std::wstring> const& mimModule,
      std::wstring const& logicalTimeImplementationName)
      -> std::optional<umbra::detail::FederationDefinition> {
    auto prepared = fomContext.coordinator.prepareCreate(
        fomModules,
        mimModule,
        logicalTimeImplementationName);
    if (!prepared.accepted()) {
      std::fprintf(
          stderr,
          "Umbra process coordinator rejected FOM creation (status=%u): %s\n",
          static_cast<unsigned int>(prepared.status),
          prepared.diagnostics.c_str());
      return std::nullopt;
    }
    return std::move(prepared.definition);
  };
  options.additionalFomPreparation = [&fomContext](
      umbra::detail::FederationDefinition const& existing,
      std::vector<std::wstring> const& additionalModules)
      -> std::optional<umbra::detail::FederationDefinition> {
    auto prepared = fomContext.coordinator.prepareAdditionalModules(
        existing,
        additionalModules);
    if (!prepared.accepted()) {
      std::fprintf(
          stderr,
          "Umbra process coordinator rejected additional FOM modules (status=%u): %s\n",
          static_cast<unsigned int>(prepared.status),
          prepared.diagnostics.c_str());
      return std::nullopt;
    }
    return std::move(prepared.definition);
  };
  return options;
}

}  // namespace

#endif

namespace umbra::detail {

int runProcessFederationService(
    std::string_view listenHost,
    std::uint16_t listenPort,
    std::filesystem::path const& serviceReportDirectory,
    ProcessFederationServiceListeningCallback onListening,
    ProcessFederationServiceStopPredicate stopRequested,
    void* userData,
    bool websocketTransport,
    bool tlsTransport,
    bool secureWebSocketTransport) noexcept {
#if !defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  static_cast<void>(listenHost);
  static_cast<void>(listenPort);
  static_cast<void>(serviceReportDirectory);
  static_cast<void>(onListening);
  static_cast<void>(stopRequested);
  static_cast<void>(userData);
  static_cast<void>(websocketTransport);
  static_cast<void>(tlsTransport);
  static_cast<void>(secureWebSocketTransport);
  std::fputs(
      "Umbra's process coordinator requires the embedded federation-management profile.\n",
      stderr);
  return 2;
#else
  try {
    if (listenHost.empty()) {
      throw std::invalid_argument("The process coordinator listen host is empty.");
    }
    if (serviceReportDirectory.empty()) {
      throw std::invalid_argument(
          "The process coordinator requires a service-report directory.");
    }
    auto const transportModes = static_cast<unsigned int>(websocketTransport) +
        static_cast<unsigned int>(tlsTransport) +
        static_cast<unsigned int>(secureWebSocketTransport);
    if (transportModes > 1U) {
      throw std::invalid_argument(
          "The process coordinator accepts exactly one transport mode.");
    }

    std::error_code pathError;
    auto reportDirectory = std::filesystem::absolute(
                               serviceReportDirectory,
                               pathError)
                               .lexically_normal();
    if (pathError) {
      throw std::runtime_error(
          "The process coordinator could not resolve its service-report directory.");
    }
    std::filesystem::create_directories(reportDirectory);
    if (!std::filesystem::is_directory(reportDirectory)) {
      throw std::runtime_error(
          "The process coordinator service-report path is not a directory.");
    }

    ProcessCoordinatorFomContext fomContext(
        processFederationResourceDirectory());
    EmbeddedFederationRegistry registry;
    auto serviceOptions = makeProcessServiceOptions(
        fomContext,
        std::move(reportDirectory));
    auto const identityEpoch = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    ProcessTransportAddress listenAddress{
        std::string(listenHost), listenPort};
    if (websocketTransport) {
      listenAddress.framing =
          ProcessTransportAddress::Framing::websocket;
      listenAddress.websocketResource = "/hla";
    }
    if (tlsTransport) {
      listenAddress.security = ProcessTransportAddress::Security::tls13;
    }
    if (secureWebSocketTransport) {
      listenAddress.security = ProcessTransportAddress::Security::tls13;
      listenAddress.framing =
          ProcessTransportAddress::Framing::websocket;
      listenAddress.websocketResource = "/hla";
    }
    ProcessFederationServiceHost host(
        registry,
        FederationDefinition{},
        std::move(listenAddress),
        TransportEndpointIdentity{
            "umbra-authoritative-process-coordinator", identityEpoch},
        std::move(serviceOptions),
        processFederationServiceHostOptionsFromRtiInitializationData());
    host.start();
    auto const boundAddress = host.address();
    if (onListening != nullptr) {
      onListening(boundAddress.host, boundAddress.port, userData);
    }

    while (stopRequested == nullptr || !stopRequested(userData)) {
      host.rethrowIfFailed();
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    host.stop();
    host.rethrowIfFailed();
    return 0;
  } catch (std::exception const& error) {
    std::fprintf(stderr, "Umbra process coordinator failed: %s\n", error.what());
    return 1;
  } catch (...) {
    std::fputs("Umbra process coordinator failed with an unknown error.\n", stderr);
    return 1;
  }
#endif
}

}  // namespace umbra::detail
