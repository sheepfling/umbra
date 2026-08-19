#pragma once

#include <filesystem>

#include <RTI/RtiConfiguration.h>

// Umbra's embedded profile has one standards-facing service-report storage
// model: a real filesystem directory.  This is deliberately a small
// implementation-defined convenience layer around the official opaque
// RtiConfiguration additional-settings field; it is not an IEEE 1516.1-2025
// binding type and exposes no production memory/backend choice.
namespace umbra::embedded {

struct ServiceReportConfiguration final {
  std::filesystem::path directory;
};

[[nodiscard]] inline rti1516_2025::RtiConfiguration makeEmbeddedRtiConfiguration(
    ServiceReportConfiguration const& serviceReports) {
  auto configuration = rti1516_2025::RtiConfiguration::createConfiguration();
  configuration.withAdditionalSettings(
      L"serviceReportDirectory=" + serviceReports.directory.wstring());
  return configuration;
}

}  // namespace umbra::embedded
