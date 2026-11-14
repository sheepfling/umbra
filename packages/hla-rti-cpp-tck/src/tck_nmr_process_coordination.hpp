#pragma once

#include "tck_main_scenarios.hpp"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace hla_rti_cpp_tck::detail {

[[nodiscard]] inline std::filesystem::path nextNmrProcessRunDirectory(
    Options const& options) {
  require(
      !options.nmrProcessCoordinationDirectory.empty(),
      "NMR process role requires an adapter coordination directory");
  static std::size_t sequence = 0U;
  auto const directory = options.nmrProcessCoordinationDirectory /
      ("run-" + std::to_string(sequence++));
  std::filesystem::create_directories(directory);
  return directory;
}

inline void writeNmrProcessMarker(
    std::filesystem::path const& directory,
    std::string const& name) {
  std::ofstream marker(directory / name, std::ios::binary | std::ios::trunc);
  require(static_cast<bool>(marker), "Could not create NMR process marker " + name);
  marker << "ready\n";
  marker.flush();
  require(static_cast<bool>(marker), "Could not write NMR process marker " + name);
}

inline void waitForNmrProcessMarker(
    std::filesystem::path const& directory,
    std::string const& name,
    Options const& options) {
  auto const deadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < deadline) {
    std::ifstream marker(directory / name, std::ios::binary);
    std::string value;
    if (marker >> value && value == "ready") {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  throw std::runtime_error("Timed out waiting for NMR process marker " + name);
}

}  // namespace hla_rti_cpp_tck::detail
