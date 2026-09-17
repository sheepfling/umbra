#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include <umbra/embedded_profile_configuration.hpp>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The synchronization-point service-report test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::FederateHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

class ScopedTemporaryDirectory final {
 public:
  explicit ScopedTemporaryDirectory(std::filesystem::path path)
      : path_(std::move(path)) {}

  ScopedTemporaryDirectory(ScopedTemporaryDirectory const&) = delete;
  ScopedTemporaryDirectory& operator=(ScopedTemporaryDirectory const&) = delete;

  ~ScopedTemporaryDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path_, ignored);
  }

  [[nodiscard]] std::filesystem::path const& path() const noexcept {
    return path_;
  }

 private:
  std::filesystem::path path_;
};

ScopedTemporaryDirectory temporaryServiceReportDirectory() {
  static std::atomic_uint64_t counter{0U};
  auto const parent = std::filesystem::temp_directory_path();
  for (std::size_t attempt = 0U; attempt != 1024U; ++attempt) {
    auto const path = parent /
        ("umbra-sync-resignation-service-report-" +
         std::to_string(++counter) + "-" + std::to_string(attempt));
    std::error_code error;
    if (std::filesystem::create_directory(path, error)) {
      return ScopedTemporaryDirectory(path);
    }
    if (error && error != std::errc::file_exists) {
      throw std::filesystem::filesystem_error(
          "Unable to reserve temporary service-report directory", path, error);
    }
  }
  throw std::runtime_error("Unable to reserve a unique temporary service-report directory.");
}

[[nodiscard]] rti1516_2025::RtiConfiguration
configurationForServiceReportDirectory(std::filesystem::path const& directory) {
  return umbra::embedded::makeEmbeddedRtiConfiguration(
      umbra::embedded::ServiceReportConfiguration{directory});
}

std::vector<std::filesystem::path> serviceReportFiles(
    std::filesystem::path const& directory) {
  std::vector<std::filesystem::path> files;
  for (auto const& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());
  return files;
}

std::string readTextFile(std::filesystem::path const& path) {
  std::ifstream input(path, std::ios::binary);
  REQUIRE(input.good());
  return {
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"service-report-federation-synchronized-resignation-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct FederationSynchronizedReport final {
    std::wstring label;
    FederateHandleSet failedToSyncSet;
  };

  void federationSynchronized(
      std::wstring const& label,
      FederateHandleSet const& failedToSyncSet) override {
    federationSynchronizedReports.push_back({label, failedToSyncSet});
  }

  std::vector<FederationSynchronizedReport> federationSynchronizedReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded service reporting records Federation Synchronized after a synchronization-set resignation",
    "[integration][development-profile][federation-management][synchronization]"
    "[mom][service-report-file][service-reporting]"
    "[federation-synchronized-service-report]"
    "[rti.service.register-federation-synchronization-point]"
    "[rti.service.synchronization-point-achieved]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.federation-synchronized]") {
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  auto first = makeRti();
  auto second = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
       "ieee1516.2-2025" / "resources" / "examples" /
       "RestaurantFOMmodule-2025.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  VariableLengthData const tag;
  auto directory = temporaryServiceReportDirectory();
  auto firstConfiguration = configurationForServiceReportDirectory(directory.path());
  auto secondConfiguration = configurationForServiceReportDirectory(directory.path());
  firstConfiguration.withRtiAddress(L"in-process");
  secondConfiguration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED, firstConfiguration));
  REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED, secondConfiguration));
  REQUIRE_NOTHROW(first->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  auto const firstHandle = first->joinFederationExecution(
      L"federation-synchronized-resign-first", L"sync", federationName);
  auto const secondHandle = second->joinFederationExecution(
      L"federation-synchronized-resign-second", L"sync", federationName);

  auto const reportFileFor = [&directory](std::string const& federateName) {
    auto const marker = std::string{"\"HLAfederateName\":\""} + federateName + "\"";
    for (auto const& candidate : serviceReportFiles(directory.path())) {
      if (readTextFile(candidate).find(marker) != std::string::npos) {
        return candidate;
      }
    }
    FAIL("joined federate service-report file was not found");
    return std::filesystem::path{};
  };
  auto const firstReportFile =
      reportFileFor("federation-synchronized-resign-first");

  FederateHandleSet synchronizationSet;
  synchronizationSet.insert(firstHandle);
  synchronizationSet.insert(secondHandle);
  REQUIRE_NOTHROW(first->registerFederationSynchronizationPoint(
      L"resignation-completion-report", tag, synchronizationSet));
  REQUIRE(first->evokeCallback(0.0));
  REQUIRE_FALSE(first->evokeCallback(0.0));
  REQUIRE_FALSE(second->evokeCallback(0.0));

  REQUIRE_NOTHROW(first->synchronizationPointAchieved(
      L"resignation-completion-report"));
  auto const firstBeforeResignation = readTextFile(firstReportFile);
  REQUIRE(firstReports.federationSynchronizedReports.empty());

  // A resigning member is removed from the synchronization set. The
  // remaining already-achieved member therefore receives §4.18 with an
  // empty failed-to-sync set, written before its callback is evoked.
  REQUIRE_NOTHROW(second->resignFederationExecution(rti1516_2025::NO_ACTION));
  auto const federationSynchronized =
      R"umbra({"HLAserialNumber":4,"HLAreturnedArgument":[null],"HLAservice":"FederationSynchronized","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Synchronization point label","HLAargumentValue":"resignation-completion-report"},{"HLAargumentType":18,"HLAargumentName":"Set of joined federate designators","HLAargumentValue":[]}],"HLAsuccessIndicator":true,"HLAexception":null})umbra";
  REQUIRE(readTextFile(firstReportFile) ==
          firstBeforeResignation + federationSynchronized);
  REQUIRE(firstReports.federationSynchronizedReports.empty());
  REQUIRE_FALSE(first->evokeCallback(0.0));
  REQUIRE(firstReports.federationSynchronizedReports.size() == 1U);
  REQUIRE(firstReports.federationSynchronizedReports.front().label ==
          L"resignation-completion-report");
  REQUIRE(firstReports.federationSynchronizedReports.front().failedToSyncSet.empty());

  REQUIRE_NOTHROW(first->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(second->disconnect());
  REQUIRE_NOTHROW(first->disconnect());
}
