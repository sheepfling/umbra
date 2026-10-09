#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include <RTI/RTI1516.h>

// Small reusable fixtures for the IEEE 1516.1-2025 connection test sources.
namespace umbra::test::connection_support_2025 {

inline std::unique_ptr<rti1516_2025::RTIambassador> makeRti() {
  rti1516_2025::RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

inline std::filesystem::path temporaryServiceReportDirectory() {
  static std::atomic_uint64_t next{0};
  // CTest starts separate processes whose counters all begin at zero. Claim
  // each directory atomically so parallel cases never share or remove it.
  for (;;) {
    auto const candidate = std::filesystem::temp_directory_path() /
        ("umbra-service-report-configuration-" + std::to_string(++next));
    if (std::filesystem::create_directory(candidate)) {
      return candidate;
    }
  }
}

}  // namespace umbra::test::connection_support_2025
