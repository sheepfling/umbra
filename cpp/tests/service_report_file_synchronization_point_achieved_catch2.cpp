#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "internal/runtime/utf8_string.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
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

using rti1516_2025::FederateHandle;
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
        ("umbra-sync-achieved-service-report-" +
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

std::string formatFederateHandleSetForReport(FederateHandleSet const& handles) {
  std::string result{"["};
  bool first = true;
  for (auto const& handle : handles) {
    auto const text = umbra::detail::utf8FromWide(handle.toString());
    REQUIRE(text.has_value());
    if (!first) {
      result += ',';
    }
    first = false;
    result += '\"';
    result += *text;
    result += '\"';
  }
  result += ']';
  return result;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"service-report-synchronization-point-achieved-" +
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
    "Embedded service reporting preserves Synchronization Point Achieved arguments",
    "[integration][development-profile][federation-management][synchronization]"
    "[mom][service-report-file][service-reporting][synchronization-point-achieved]"
    "[rti.service.register-federation-synchronization-point]"
    "[rti.service.synchronization-point-achieved]"
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
  unsigned char const syncTagBytes[] = {0x53, 0x59, 0x4E, 0x43};
  VariableLengthData const syncTag(syncTagBytes, sizeof(syncTagBytes));
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
      L"sync-report-first", L"sync", federationName);
  auto const secondHandle = second->joinFederationExecution(
      L"sync-report-second", L"sync", federationName);

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
  auto const firstReportFile = reportFileFor("sync-report-first");
  auto const secondReportFile = reportFileFor("sync-report-second");
  auto const firstInitialText = readTextFile(firstReportFile);
  auto const secondInitialText = readTextFile(secondReportFile);

  FederateHandleSet synchronizationSet;
  synchronizationSet.insert(firstHandle);
  synchronizationSet.insert(secondHandle);
  REQUIRE_NOTHROW(first->registerFederationSynchronizationPoint(
      L"report-startup", syncTag, synchronizationSet));
  auto const registrationRecord =
      std::string{R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"RegisterFederationSynchronizationPoint","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Synchronization point label","HLAargumentValue":"report-startup"},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"U1lOQw=="},{"HLAargumentType":18,"HLAargumentName":"Optional set of joined federate designators","HLAargumentValue":)"} +
      formatFederateHandleSetForReport(synchronizationSet) +
      R"(}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const registrationConfirmation =
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"ConfirmSynchronizationPointRegistration","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Synchronization point label","HLAargumentValue":"report-startup"},{"HLAargumentType":6,"HLAargumentName":"Registration-success indicator","HLAargumentValue":true},{"HLAargumentType":34,"HLAargumentName":"Optional failure reason","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const firstAnnouncement =
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"AnnounceSynchronizationPoint","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Synchronization point label","HLAargumentValue":"report-startup"},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"U1lOQw=="}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const secondAnnouncement =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"AnnounceSynchronizationPoint","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Synchronization point label","HLAargumentValue":"report-startup"},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"U1lOQw=="}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(firstReportFile) ==
          firstInitialText + registrationRecord + registrationConfirmation +
              firstAnnouncement);
  REQUIRE(readTextFile(secondReportFile) ==
          secondInitialText + secondAnnouncement);
  REQUIRE(first->evokeCallback(0.0));
  REQUIRE_FALSE(first->evokeCallback(0.0));
  REQUIRE_FALSE(second->evokeCallback(0.0));

  REQUIRE_NOTHROW(first->synchronizationPointAchieved(L"report-startup"));
  auto const firstRecord =
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[null],"HLAservice":"SynchronizationPointAchieved","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Synchronization point label","HLAargumentValue":"report-startup"},{"HLAargumentType":6,"HLAargumentName":"Optional synchronization-success indicator","HLAargumentValue":true}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(firstReportFile) ==
          firstInitialText + registrationRecord + registrationConfirmation +
              firstAnnouncement + firstRecord);
  REQUIRE(readTextFile(secondReportFile) == secondInitialText + secondAnnouncement);
  REQUIRE(firstReports.federationSynchronizedReports.empty());
  REQUIRE(secondReports.federationSynchronizedReports.empty());

  REQUIRE_NOTHROW(second->synchronizationPointAchieved(
      L"report-startup", false));
  auto const secondRecord =
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"SynchronizationPointAchieved","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Synchronization point label","HLAargumentValue":"report-startup"},{"HLAargumentType":6,"HLAargumentName":"Optional synchronization-success indicator","HLAargumentValue":false}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const federationSynchronizedRecord = [&secondHandle](
                                                std::uint32_t serialNumber) {
    auto const handleSet = FederateHandleSet{secondHandle};
    return std::string{R"({"HLAserialNumber":)"} +
        std::to_string(serialNumber) +
        R"(,"HLAreturnedArgument":[null],"HLAservice":"FederationSynchronized","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Synchronization point label","HLAargumentValue":"report-startup"},{"HLAargumentType":18,"HLAargumentName":"Set of joined federate designators","HLAargumentValue":)" +
        formatFederateHandleSetForReport(handleSet) +
        R"(}],"HLAsuccessIndicator":true,"HLAexception":null})";
  };
  auto const firstFederationSynchronized = federationSynchronizedRecord(4U);
  auto const secondFederationSynchronized = federationSynchronizedRecord(2U);
  // In HLA_EVOKED mode, both successful file records must already be durable
  // before the last achievement's Federation Synchronized callback is
  // dispatched. The RTI-generated callback is itself a selected service
  // report for the remaining federate and therefore advances its serial.
  REQUIRE(readTextFile(firstReportFile) ==
          firstInitialText + registrationRecord + registrationConfirmation +
              firstAnnouncement + firstRecord + firstFederationSynchronized);
  REQUIRE(readTextFile(secondReportFile) ==
          secondInitialText + secondAnnouncement + secondRecord +
              secondFederationSynchronized);
  REQUIRE(firstReports.federationSynchronizedReports.empty());
  REQUIRE(secondReports.federationSynchronizedReports.empty());

  REQUIRE_FALSE(first->evokeCallback(0.0));
  REQUIRE_FALSE(second->evokeCallback(0.0));
  REQUIRE(firstReports.federationSynchronizedReports.size() == 1U);
  REQUIRE(secondReports.federationSynchronizedReports.size() == 1U);
  REQUIRE(firstReports.federationSynchronizedReports.front().label ==
          L"report-startup");
  REQUIRE(secondReports.federationSynchronizedReports.front().label ==
          L"report-startup");
  REQUIRE(secondReports.federationSynchronizedReports.front().failedToSyncSet.contains(
      secondHandle));

  // A rejected invocation after completion has no successful-void record.
  REQUIRE_THROWS_AS(
      first->synchronizationPointAchieved(L"report-startup"),
      rti1516_2025::SynchronizationPointLabelNotAnnounced);
  REQUIRE(readTextFile(firstReportFile) ==
          firstInitialText + registrationRecord + registrationConfirmation +
              firstAnnouncement + firstRecord + firstFederationSynchronized);

  REQUIRE_NOTHROW(second->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(first->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(second->disconnect());
  REQUIRE_NOTHROW(first->disconnect());
}
