#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>

#include <RTI/RTI1516.h>

// Small reusable fixtures for the IEEE 1516.1-2025 connection test sources.
namespace umbra::test::connection_support_2025 {

inline std::unique_ptr<rti1516_2025::RTIambassador> makeRti() {
  rti1516_2025::RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

inline std::filesystem::path temporaryServiceReportDirectory() {
  static std::atomic_uint64_t next{0};
  static std::uint64_t const processRun = [] {
    auto const ticks = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    auto const thread = static_cast<std::uint64_t>(
        std::hash<std::thread::id>{}(std::this_thread::get_id()));
    return ticks ^ (thread + (ticks << 6U) + (ticks >> 2U));
  }();
  auto const parent = std::filesystem::temp_directory_path();
  for (std::size_t attempt = 0U; attempt != 1024U; ++attempt) {
    auto const candidate = parent /
        ("umbra-rpt-" + std::to_string(processRun) + "-" +
         std::to_string(++next) + "-" + std::to_string(attempt));
    std::error_code error;
    if (std::filesystem::create_directory(candidate, error)) {
      return candidate;
    }
    std::error_code statusError;
    if (std::filesystem::exists(candidate, statusError)) {
      continue;
    }
    if (statusError) {
      throw std::filesystem::filesystem_error(
          "Could not inspect a service-report directory candidate",
          candidate,
          statusError);
    }
    if (error && error != std::errc::file_exists) {
      throw std::filesystem::filesystem_error(
          "Could not allocate a service-report directory", candidate, error);
    }
  }
  throw std::runtime_error("Could not allocate a unique service-report directory.");
}

}  // namespace umbra::test::connection_support_2025
