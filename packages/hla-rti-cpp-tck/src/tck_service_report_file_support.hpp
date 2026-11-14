#pragma once

#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace hla_rti_cpp_tck::detail::service_report_file_support {

inline std::wstring serviceReportSettings(Options const& options) {
  auto const settings = toWide(options.additionalSettings);
  constexpr std::wstring_view directorySetting = L"serviceReportDirectory=";
  std::wstring result;
  auto append = [&](std::wstring_view token) {
    if (token.empty()) {
      return;
    }
    if (!result.empty()) {
      result.push_back(L';');
    }
    result.append(token);
  };

  std::size_t begin = 0U;
  while (begin <= settings.size()) {
    auto const end = settings.find(L';', begin);
    auto const token = std::wstring_view(settings).substr(
        begin,
        end == std::wstring::npos ? std::wstring::npos : end - begin);
    if (!token.starts_with(directorySetting)) {
      append(token);
    }
    if (end == std::wstring::npos) {
      break;
    }
    begin = end + 1U;
  }

  append(directorySetting);
  result.append(options.serviceReportDirectory.wstring());
  return result;
}

inline void connectWithServiceReportDirectory(
    Session& session,
    Options const& options,
    bool ownerConfiguration = false) {
  auto configuration = rti::RtiConfiguration::createConfiguration();
  auto const& configuredName = ownerConfiguration
      ? options.ownerConfigurationName
      : options.memberConfigurationName;
  auto const configurationName = configuredName.empty()
      ? options.configurationName
      : configuredName;
  if (!configurationName.empty()) {
    configuration.withConfigurationName(toWide(configurationName));
  }
  if (!options.rtiAddress.empty()) {
    configuration.withRtiAddress(toWide(options.rtiAddress));
  }
  configuration.withAdditionalSettings(serviceReportSettings(options));
  static_cast<void>(session.connectWithConfiguration(configuration));
}

inline std::optional<std::filesystem::path> advertisedServiceReportFile(
    Recorder const& recorder,
    rti::AttributeHandle const& federateNameAttribute,
    rti::AttributeHandle const& reportFileAttribute,
    std::wstring const& federateName) {
  for (auto const& reflection : recorder.reflections()) {
    auto const name = std::find_if(
        reflection.values.begin(), reflection.values.end(),
        [&](auto const& value) { return value.first == federateNameAttribute; });
    auto const reportFile = std::find_if(
        reflection.values.begin(), reflection.values.end(),
        [&](auto const& value) { return value.first == reportFileAttribute; });
    if (name == reflection.values.end() || reportFile == reflection.values.end()) {
      continue;
    }
    rti::HLAunicodeString decodedName;
    decodedName.decode(name->second);
    if (decodedName.get() != federateName) {
      continue;
    }
    rti::HLAunicodeString decodedReportFile;
    decodedReportFile.decode(reportFile->second);
    if (decodedReportFile.get().empty()) {
      throw std::runtime_error(
          "Publisher Federate MOM advertised an empty HLAreportServiceFile value");
    }
    return std::filesystem::path(decodedReportFile.get());
  }
  return std::nullopt;
}

inline std::string readServiceReportFile(std::filesystem::path const& file) {
  std::ifstream input(file, std::ios::binary);
  if (!input) {
    throw std::runtime_error(
        "Unable to read the file advertised by HLAreportServiceFile");
  }
  return std::string(
      std::istreambuf_iterator<char>{input},
      std::istreambuf_iterator<char>{});
}

}  // namespace hla_rti_cpp_tck::detail::service_report_file_support
