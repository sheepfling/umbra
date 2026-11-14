#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace {

std::filesystem::path temporaryProcessDirectory() {
  static std::atomic_uint64_t sequence{0U};
  auto const stamp = std::chrono::high_resolution_clock::now()
                         .time_since_epoch()
                         .count();
  return std::filesystem::temp_directory_path() /
      ("umbra-process-service-" + std::to_string(stamp) + "-" +
       std::to_string(++sequence));
}

std::string quoted(std::filesystem::path const& path) {
  // The probe and temporary directory are generated paths.  Quoting the full
  // argument keeps the coordinator correct when the Windows workspace or
  // temporary root contains spaces.
  return "\"" + path.string() + "\"";
}

std::string marker(std::filesystem::path const& path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::filesystem::path temporaryCoordinatorDirectory() {
  static std::atomic_uint64_t sequence{0U};
  auto const stamp = std::chrono::high_resolution_clock::now()
                         .time_since_epoch()
                         .count();
  return std::filesystem::temp_directory_path() /
      ("umbra-authoritative-coordinator-" + std::to_string(stamp) + "-" +
       std::to_string(++sequence));
}

class CoordinatorProcess final {
 public:
  explicit CoordinatorProcess(std::filesystem::path directory)
      : directory_(std::move(directory)),
        process_(std::async(std::launch::async, [this] {
          auto command = quoted(UMBRA_PROCESS_SERVICE_PROBE_PATH) +
              " coordinator-runtime " + quoted(directory_);
#if defined(_WIN32)
          command = std::string{"\""} + command + "\"";
#endif
          return std::system(command.c_str());
        })) {}

  CoordinatorProcess(CoordinatorProcess const&) = delete;
  CoordinatorProcess& operator=(CoordinatorProcess const&) = delete;

  ~CoordinatorProcess() {
    requestStop();
    if (process_.valid()) {
      static_cast<void>(process_.wait_for(std::chrono::seconds(5)));
      if (process_.valid() &&
          process_.wait_for(std::chrono::milliseconds(0)) ==
              std::future_status::ready) {
        static_cast<void>(process_.get());
      }
    }
    std::error_code ignored;
    std::filesystem::remove_all(directory_, ignored);
  }

  [[nodiscard]] bool waitUntilListening(std::chrono::milliseconds timeout) const {
    auto const deadline = std::chrono::steady_clock::now() + timeout;
    auto const addressPath = directory_ / "coordinator.address";
    while (std::chrono::steady_clock::now() < deadline) {
      if (std::filesystem::is_regular_file(addressPath)) {
        return true;
      }
      if (process_.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
        return false;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return std::filesystem::is_regular_file(addressPath);
  }

  [[nodiscard]] std::pair<std::string, std::uint16_t> address() const {
    std::ifstream input(directory_ / "coordinator.address", std::ios::binary);
    std::string host;
    unsigned int port = 0U;
    input >> host >> port;
    if (!input || port == 0U || port > UINT16_MAX) {
      throw std::runtime_error("The production coordinator published an invalid address.");
    }
    return {std::move(host), static_cast<std::uint16_t>(port)};
  }

  void requestStop() const noexcept {
    try {
      std::ofstream output(
          directory_ / "coordinator.stop",
          std::ios::binary | std::ios::trunc);
      output << "stop\n";
    } catch (...) {
    }
  }

  [[nodiscard]] int stopAndGetExitCode() {
    requestStop();
    if (process_.wait_for(std::chrono::seconds(5)) !=
        std::future_status::ready) {
      throw std::runtime_error("The production coordinator did not stop on request.");
    }
    return process_.get();
  }

  [[nodiscard]] std::filesystem::path const& directory() const noexcept {
    return directory_;
  }

 private:
  std::filesystem::path directory_;
  std::future<int> process_;
};

class CoordinatorClientProcess final {
 public:
  explicit CoordinatorClientProcess(std::filesystem::path directory)
      : directory_(std::move(directory)),
        process_(std::async(std::launch::async, [this] {
          auto command = quoted(UMBRA_PROCESS_SERVICE_PROBE_PATH) +
              " coordinator-client " + quoted(directory_);
#if defined(_WIN32)
          command = std::string{"\""} + command + "\"";
#endif
          return std::system(command.c_str());
        })) {}

  CoordinatorClientProcess(CoordinatorClientProcess const&) = delete;
  CoordinatorClientProcess& operator=(CoordinatorClientProcess const&) = delete;

  ~CoordinatorClientProcess() {
    requestRelease();
    if (process_.valid()) {
      static_cast<void>(process_.wait_for(std::chrono::seconds(5)));
      if (process_.valid() &&
          process_.wait_for(std::chrono::milliseconds(0)) ==
              std::future_status::ready) {
        static_cast<void>(process_.get());
      }
    }
  }

  [[nodiscard]] bool waitUntilJoined(
      std::chrono::milliseconds timeout) const {
    auto const deadline = std::chrono::steady_clock::now() + timeout;
    auto const joinedPath = directory_ / "coordinator-client.joined";
    while (std::chrono::steady_clock::now() < deadline) {
      if (std::filesystem::is_regular_file(joinedPath)) {
        return true;
      }
      if (process_.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
        return false;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return std::filesystem::is_regular_file(joinedPath);
  }

  [[nodiscard]] bool stillRunning() const {
    return process_.wait_for(std::chrono::milliseconds(0)) !=
        std::future_status::ready;
  }

  [[nodiscard]] bool waitUntilConnectionLost(
      std::chrono::milliseconds timeout) const {
    auto const deadline = std::chrono::steady_clock::now() + timeout;
    auto const lostPath = directory_ / "coordinator-client.connection-lost";
    while (std::chrono::steady_clock::now() < deadline) {
      if (std::filesystem::is_regular_file(lostPath)) {
        return true;
      }
      if (process_.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
        return false;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return std::filesystem::is_regular_file(lostPath);
  }

  void requestRelease() const noexcept {
    try {
      std::ofstream output(
          directory_ / "coordinator-client.release",
          std::ios::binary | std::ios::trunc);
      output << "release\n";
    } catch (...) {
    }
  }

  [[nodiscard]] int releaseAndGetExitCode() {
    requestRelease();
    if (process_.wait_for(std::chrono::seconds(5)) !=
        std::future_status::ready) {
      throw std::runtime_error(
          "The independent production coordinator client did not stop on request.");
    }
    return process_.get();
  }

  [[nodiscard]] int waitForExitAndGetExitCode(
      std::chrono::milliseconds timeout) {
    if (process_.wait_for(timeout) != std::future_status::ready) {
      throw std::runtime_error(
          "The coordinator client did not exit after connectionLost.");
    }
    return process_.get();
  }

 private:
  std::filesystem::path directory_;
  std::future<int> process_;
};

class CoordinatorConnectionLostAmbassador final
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

std::unique_ptr<rti1516_2025::RTIambassador> makePublicRti() {
  rti1516_2025::RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Private registry-bound service exchanges federation traffic across independently launched processes",
    "[integration][foundation][transport][process-boundary][service-dispatch][registry-binding][transport-contract][interaction-management][2025]") {
#ifndef UMBRA_PROCESS_SERVICE_PROBE_PATH
  FAIL("The process service probe path was not configured by CMake.");
#else
  auto const directory = temporaryProcessDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  REQUIRE(std::filesystem::create_directories(directory));

  std::filesystem::path const probe = UMBRA_PROCESS_SERVICE_PROBE_PATH;
  REQUIRE(std::filesystem::is_regular_file(probe));

  auto const launch = [&](std::string const& role) {
    auto command = quoted(probe) + " " + role + " " + quoted(directory);
#if defined(_WIN32)
    // std::system delegates to cmd.exe on Windows.  The additional outer
    // quotes are required by cmd.exe when the executable path is quoted.
    command = std::string{"\""} + command + "\"";
#endif
    return std::async(
        std::launch::async,
        [command] { return std::system(command.c_str()); });
  };

  // The clients poll the server's published ephemeral port, so all three
  // processes can be launched without a sleep-based sequencing assumption.
  auto server = launch("server");
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  auto sender = launch("sender");
  auto receiver = launch("receiver");

  auto const senderStatus = sender.get();
  auto const receiverStatus = receiver.get();
  auto const serverStatus = server.get();
  REQUIRE(senderStatus == 0);
  REQUIRE(receiverStatus == 0);
  REQUIRE(serverStatus == 0);
  REQUIRE(marker(directory / "sender.ok") == "ok\n");
  REQUIRE(marker(directory / "receiver.ok") == "callback-ok\n");
  REQUIRE(marker(directory / "server.ok") == "ok\n");

  std::filesystem::remove_all(directory, ignored);
#endif
}

TEST_CASE(
    "Production coordinator runtime serves two independent public 2025 API clients",
    "[integration][internal][2025][process-boundary][authoritative-coordinator]"
    "[process-profile-runtime]") {
#ifndef UMBRA_PROCESS_SERVICE_PROBE_PATH
  FAIL("The process service probe path was not configured by CMake.");
#else
  auto const directory = temporaryCoordinatorDirectory();
  REQUIRE(std::filesystem::create_directories(directory));
  CoordinatorProcess coordinator(directory);
  REQUIRE(coordinator.waitUntilListening(std::chrono::seconds(5)));
  auto const [host, port] = coordinator.address();

  rti1516_2025::NullFederateAmbassador federateAmbassador;
  auto rti = makePublicRti();
  auto configuration = rti1516_2025::RtiConfiguration::createConfiguration()
      .withConfigurationName(L"independent-process-coordinator-client")
      .withRtiAddress(L"tcp://" + std::wstring(host.begin(), host.end()) +
                      L":" + std::to_wstring(port));
  auto connectionResult = [&]() -> rti1516_2025::ConfigurationResult {
    try {
      return rti->connect(
          federateAmbassador,
          rti1516_2025::HLA_EVOKED,
          configuration);
    } catch (rti1516_2025::Exception const& error) {
      auto const name = error.name();
      auto const detail = error.what();
      throw std::runtime_error(
          "Public Connect failed: " + std::string(name.begin(), name.end()) +
          ": " + std::string(detail.begin(), detail.end()));
    } catch (...) {
      throw std::runtime_error(
          "Public Connect raised a non-standard exception.");
    }
  }();
  REQUIRE(connectionResult.addressUsed);

  auto const federationName = L"independent-process-coordinator-smoke";
  auto const fomPath = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / "examples" /
      "RestaurantFOMmodule-2025.xml";
  try {
    rti->createFederationExecution(
        federationName,
        fomPath.wstring(),
        L"HLAinteger64Time");
  } catch (rti1516_2025::Exception const& error) {
    auto const name = error.name();
    auto const detail = error.what();
    throw std::runtime_error(
        "Public Create Federation Execution failed: " +
        std::string(name.begin(), name.end()) + ": " +
        std::string(detail.begin(), detail.end()));
  }
  try {
    static_cast<void>(rti->joinFederationExecution(
        L"coordinator-parent-federate",
        L"process-runtime-smoke",
        federationName));
  } catch (rti1516_2025::Exception const& error) {
    auto const name = error.name();
    auto const detail = error.what();
    throw std::runtime_error(
        "Public Join Federation Execution failed: " +
        std::string(name.begin(), name.end()) + ": " +
        std::string(detail.begin(), detail.end()));
  }

  {
    std::ofstream federationRequest(
        directory / "coordinator-client.federation",
        std::ios::binary | std::ios::trunc);
    federationRequest << "independent-process-coordinator-smoke\n";
    federationRequest.close();
    REQUIRE(static_cast<bool>(federationRequest));
  }
  CoordinatorClientProcess client(directory);
  INFO(marker(directory / "coordinator-client.error"));
  REQUIRE(client.waitUntilJoined(std::chrono::seconds(5)));
  REQUIRE(client.stillRunning());
  REQUIRE(client.releaseAndGetExitCode() == 0);
  REQUIRE(marker(directory / "coordinator-client.done") ==
          "joined\nresigned\n");

  rti->resignFederationExecution(rti1516_2025::NO_ACTION);
  rti->destroyFederationExecution(federationName);
  rti->disconnect();

  auto const reportDirectory = directory / "service-reports";
  REQUIRE(std::filesystem::is_directory(reportDirectory));
  auto const reportCount = static_cast<std::size_t>(std::distance(
      std::filesystem::directory_iterator(reportDirectory),
      std::filesystem::directory_iterator{}));
  REQUIRE(reportCount == 2U);
  REQUIRE(coordinator.stopAndGetExitCode() == 0);
#endif
}

TEST_CASE(
    "Production coordinator stop reports connectionLost to joined independent public clients",
    "[integration][internal][2025][process-boundary][authoritative-coordinator]"
    "[process-profile-runtime]") {
#ifndef UMBRA_PROCESS_SERVICE_PROBE_PATH
  FAIL("The process service probe path was not configured by CMake.");
#else
  auto const directory = temporaryCoordinatorDirectory();
  REQUIRE(std::filesystem::create_directories(directory));
  CoordinatorProcess coordinator(directory);
  REQUIRE(coordinator.waitUntilListening(std::chrono::seconds(5)));
  auto const [host, port] = coordinator.address();

  CoordinatorConnectionLostAmbassador federateAmbassador;
  auto rti = makePublicRti();
  auto configuration = rti1516_2025::RtiConfiguration::createConfiguration()
      .withConfigurationName(L"coordinator-authority-stop-parent")
      .withRtiAddress(L"tcp://" + std::wstring(host.begin(), host.end()) +
                      L":" + std::to_wstring(port))
      .withAdditionalSettings(L"FED_TIMEOUT_RECONNECT=0.25");
  auto const connectionResult = rti->connect(
      federateAmbassador,
      rti1516_2025::HLA_EVOKED,
      configuration);
  REQUIRE(connectionResult.addressUsed);

  auto const federationName = L"coordinator-authority-stop-smoke";
  auto const fomPath = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / "examples" /
      "RestaurantFOMmodule-2025.xml";
  rti->createFederationExecution(
      federationName,
      fomPath.wstring(),
      L"HLAinteger64Time");
  static_cast<void>(rti->joinFederationExecution(
      L"coordinator-stop-parent-federate",
      L"process-runtime-smoke",
      federationName));

  {
    std::ofstream federationRequest(
        directory / "coordinator-client.federation",
        std::ios::binary | std::ios::trunc);
    federationRequest << "coordinator-authority-stop-smoke\n";
    federationRequest.close();
    REQUIRE(static_cast<bool>(federationRequest));
  }
  CoordinatorClientProcess client(directory);
  INFO(marker(directory / "coordinator-client.error"));
  REQUIRE(client.waitUntilJoined(std::chrono::seconds(5)));
  REQUIRE(client.stillRunning());

  coordinator.requestStop();
  REQUIRE(coordinator.stopAndGetExitCode() == 0);
  auto const connectionLostObserved =
      client.waitUntilConnectionLost(std::chrono::seconds(5));
  static_cast<void>(rti->evokeMultipleCallbacks(0.0, 0.01));
  INFO("Coordinator client error: " +
       marker(directory / "coordinator-client.error"));
  REQUIRE(connectionLostObserved);
  REQUIRE(client.waitForExitAndGetExitCode(std::chrono::seconds(5)) == 0);
  REQUIRE(federateAmbassador.hasConnectionLost());
  REQUIRE_FALSE(federateAmbassador.faultDescription().empty());
  REQUIRE(marker(directory / "coordinator-client.connection-lost") ==
          "connectionLost\n");
#endif
}
