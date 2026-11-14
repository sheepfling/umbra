#include "process_federation_service_client_probe.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <thread>

namespace {

class CoordinatorClientAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  void connectionLost(std::wstring const& faultDescription) override {
    connectionLost_ = true;
    faultDescription_ = faultDescription;
  }

  [[nodiscard]] bool hasConnectionLost() const noexcept {
    return connectionLost_;
  }

  [[nodiscard]] std::wstring const& faultDescription() const noexcept {
    return faultDescription_;
  }

 private:
  bool connectionLost_ = false;
  std::wstring faultDescription_;
};

enum class WaitForClientReleaseResult {
  released,
  connectionLost,
  timedOut,
};

bool publishTextFile(
    std::filesystem::path const& path,
    std::string_view contents) noexcept {
  try {
    auto temporaryPath = path;
    temporaryPath += ".tmp";
    std::ofstream output(
        temporaryPath,
        std::ios::binary | std::ios::trunc);
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.close();
    if (!output) {
      return false;
    }
    std::filesystem::rename(temporaryPath, path);
    return true;
  } catch (...) {
    return false;
  }
}

WaitForClientReleaseResult waitForReleaseOrConnectionLoss(
    std::filesystem::path const& directory,
    rti1516_2025::RTIambassador& rti,
    CoordinatorClientAmbassador& federateAmbassador) {
  auto const releasePath = directory / "coordinator-client.release";
  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::seconds(10);
  while (std::chrono::steady_clock::now() < deadline) {
    if (federateAmbassador.hasConnectionLost()) {
      return WaitForClientReleaseResult::connectionLost;
    }
    std::error_code error;
    if (std::filesystem::exists(releasePath, error)) {
      return error ? WaitForClientReleaseResult::timedOut
                   : WaitForClientReleaseResult::released;
    }
    if (error) {
      return WaitForClientReleaseResult::timedOut;
    }
    static_cast<void>(rti.evokeMultipleCallbacks(0.0, 0.01));
  }
  return federateAmbassador.hasConnectionLost()
      ? WaitForClientReleaseResult::connectionLost
      : WaitForClientReleaseResult::timedOut;
}

}  // namespace

int runProcessFederationServiceClientProbe(
    std::filesystem::path const& directory) noexcept {
  try {
    std::ifstream addressInput(
        directory / "coordinator.address",
        std::ios::binary);
    std::string host;
    unsigned int port = 0U;
    addressInput >> host >> port;
    if (!addressInput || host.empty() || port == 0U || port > UINT16_MAX) {
      static_cast<void>(publishTextFile(
          directory / "coordinator-client.error",
          "The production coordinator address was invalid.\n"));
      return 2;
    }

    std::ifstream federationInput(
        directory / "coordinator-client.federation",
        std::ios::binary);
    std::string federationName;
    federationInput >> federationName;
    if (!federationInput || federationName.empty()) {
      static_cast<void>(publishTextFile(
          directory / "coordinator-client.error",
          "The coordinator client federation was not configured.\n"));
      return 3;
    }

    rti1516_2025::RTIambassadorFactory factory;
    auto rti = factory.createRTIambassador();
    CoordinatorClientAmbassador federateAmbassador;
    auto const wideHost = std::wstring(host.begin(), host.end());
    auto configuration =
        rti1516_2025::RtiConfiguration::createConfiguration()
            .withConfigurationName(L"independent-process-coordinator-client")
            .withRtiAddress(
                L"tcp://" + wideHost + L":" + std::to_wstring(port))
            .withAdditionalSettings(L"FED_TIMEOUT_RECONNECT=0.25");
    auto const connectionResult = rti->connect(
        federateAmbassador,
        rti1516_2025::HLA_EVOKED,
        configuration);
    if (!connectionResult.addressUsed) {
      static_cast<void>(publishTextFile(
          directory / "coordinator-client.error",
          "Public Connect did not select the coordinator address.\n"));
      return 4;
    }

    auto const wideFederation =
        std::wstring(federationName.begin(), federationName.end());
    static_cast<void>(rti->joinFederationExecution(
        L"independent-process-coordinator-federate",
        L"process-runtime-smoke",
        wideFederation));
    if (!publishTextFile(
            directory / "coordinator-client.joined",
            "joined\n")) {
      return 5;
    }

    auto const waitResult = waitForReleaseOrConnectionLoss(
        directory,
        *rti,
        federateAmbassador);
    if (waitResult == WaitForClientReleaseResult::connectionLost) {
      return publishTextFile(
                 directory / "coordinator-client.connection-lost",
                 "connectionLost\n")
          ? 0
          : 7;
    }
    if (waitResult != WaitForClientReleaseResult::released) {
      static_cast<void>(publishTextFile(
          directory / "coordinator-client.error",
          "The coordinator client did not receive its release marker.\n"));
      return 6;
    }

    rti->resignFederationExecution(rti1516_2025::NO_ACTION);
    rti->disconnect();
    if (!publishTextFile(
            directory / "coordinator-client.done",
            "joined\nresigned\n")) {
      return 7;
    }
    return 0;
  } catch (rti1516_2025::Exception const& error) {
    auto const name = error.name();
    auto const detail = error.what();
    auto message =
        "RTI exception " + std::string(name.begin(), name.end()) + ": ";
    message.append(detail.begin(), detail.end());
    message.push_back('\n');
    static_cast<void>(publishTextFile(
        directory / "coordinator-client.error",
        message));
  } catch (std::exception const& error) {
    static_cast<void>(publishTextFile(
        directory / "coordinator-client.error",
        std::string(error.what()) + "\n"));
  } catch (...) {
    static_cast<void>(publishTextFile(
        directory / "coordinator-client.error",
        "The coordinator client failed with a non-standard exception.\n"));
  }
  return 1;
}
