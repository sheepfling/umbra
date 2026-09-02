#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <string>
#include <thread>

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

}  // namespace

TEST_CASE(
    "Private registry-bound service exchanges federation traffic across independently launched processes",
    "[integration][foundation][transport][process-boundary][service-dispatch][registry-binding][transport-contract][interaction-management]") {
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
