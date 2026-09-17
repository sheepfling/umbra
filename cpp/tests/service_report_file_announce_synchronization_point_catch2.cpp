#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

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
        ("umbra-sync-announce-service-report-" +
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

std::vector<unsigned char> variableLengthDataBytes(VariableLengthData const& value) {
  auto const* begin = static_cast<unsigned char const*>(value.data());
  return {begin, begin + value.size()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"service-report-announce-synchronization-point-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct SynchronizationPointAnnouncementReport final {
    std::wstring label;
    VariableLengthData userSuppliedTag;
  };

  void announceSynchronizationPoint(
      std::wstring const& label,
      VariableLengthData const& userSuppliedTag) override {
    synchronizationPointAnnouncementReports.push_back({label, userSuppliedTag});
  }

  std::vector<SynchronizationPointAnnouncementReport>
      synchronizationPointAnnouncementReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded service reporting records Announce Synchronization Point at recipient files",
    "[integration][development-profile][federation-management][synchronization]"
    "[mom][service-report-file][service-reporting]"
    "[announce-synchronization-point-service-report]"
    "[rti.service.register-federation-synchronization-point]"
    "[federate.callback.announce-synchronization-point]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador lateReports;
  auto owner = makeRti();
  auto late = makeRti();
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
  unsigned char const tagBytes[] = {0x4c, 0x41, 0x54, 0x45};
  std::vector<unsigned char> const expectedTag{
      std::begin(tagBytes), std::end(tagBytes)};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto directory = temporaryServiceReportDirectory();
  auto ownerConfiguration = configurationForServiceReportDirectory(directory.path());
  auto lateConfiguration = configurationForServiceReportDirectory(directory.path());
  ownerConfiguration.withRtiAddress(L"in-process");
  lateConfiguration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED, ownerConfiguration));
  REQUIRE_NOTHROW(late->connect(lateReports, HLA_EVOKED, lateConfiguration));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"announce-report-owner", L"sync", federationName));

  // The globally registered point makes a later join an eligible §4.16
  // recipient. Its endpoint must already be attached before the initial
  // announcement is planned and queued.
  REQUIRE_NOTHROW(owner->registerFederationSynchronizationPoint(
      L"late-before-callback", tag));
  REQUIRE_NOTHROW(late->joinFederationExecution(
      L"announce-report-late", L"sync", federationName));

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
  auto const lateReportFile = reportFileFor("announce-report-late");
  auto const announcementRecord = [](std::uint32_t serialNumber, char const* label) {
    return std::string{R"({"HLAserialNumber":)"} +
        std::to_string(serialNumber) +
        R"(,"HLAreturnedArgument":[null],"HLAservice":"AnnounceSynchronizationPoint","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Synchronization point label","HLAargumentValue":")" +
        label +
        R"("},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"TEFURQ=="}],"HLAsuccessIndicator":true,"HLAexception":null})";
  };
  auto const lateAnnouncement = announcementRecord(0U, "late-before-callback");
  auto const lateTextAfterJoin = readTextFile(lateReportFile);
  REQUIRE(lateTextAfterJoin.size() > lateAnnouncement.size());
  auto const lateInitialText = lateTextAfterJoin.substr(
      0U,
      lateTextAfterJoin.size() - lateAnnouncement.size());
  REQUIRE(lateTextAfterJoin == lateInitialText + lateAnnouncement);
  REQUIRE(serviceReportFiles(directory.path()).size() == 2U);
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.empty());

  REQUIRE_FALSE(late->evokeCallback(0.0));
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.size() == 1U);
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.front().label ==
          L"late-before-callback");
  REQUIRE(variableLengthDataBytes(
              lateReports.synchronizationPointAnnouncementReports.front().userSuppliedTag) ==
          expectedTag);

  // The two selecting switches do not report themselves. Disabling the
  // recipient's service switch must only gate its subsequent §4.16 append;
  // it cannot suppress the actual callback or replace the joined-federate
  // file identity.
  REQUIRE(late->getServiceReportingSwitch());
  REQUIRE_NOTHROW(late->setServiceReportingSwitch(false));
  REQUIRE_FALSE(late->getServiceReportingSwitch());
  REQUIRE_NOTHROW(owner->registerFederationSynchronizationPoint(
      L"while-file-disabled", tag));
  REQUIRE(readTextFile(lateReportFile) == lateInitialText + lateAnnouncement);
  REQUIRE(serviceReportFiles(directory.path()).size() == 2U);
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.size() == 1U);
  REQUIRE_FALSE(late->evokeCallback(0.0));
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.size() == 2U);
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.back().label ==
          L"while-file-disabled");
  REQUIRE(variableLengthDataBytes(
              lateReports.synchronizationPointAnnouncementReports.back().userSuppliedTag) ==
          expectedTag);

  REQUIRE_NOTHROW(late->setServiceReportingSwitch(true));
  REQUIRE(late->getServiceReportingSwitch());
  REQUIRE(readTextFile(lateReportFile) == lateInitialText + lateAnnouncement);
  REQUIRE_NOTHROW(owner->registerFederationSynchronizationPoint(
      L"after-file-reenabled", tag));
  auto const reenabledAnnouncement =
      announcementRecord(1U, "after-file-reenabled");
  REQUIRE(readTextFile(lateReportFile) ==
          lateInitialText + lateAnnouncement + reenabledAnnouncement);
  REQUIRE(serviceReportFiles(directory.path()).size() == 2U);
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.size() == 2U);
  REQUIRE_FALSE(late->evokeCallback(0.0));
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.size() == 3U);
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.back().label ==
          L"after-file-reenabled");
  REQUIRE(variableLengthDataBytes(
              lateReports.synchronizationPointAnnouncementReports.back().userSuppliedTag) ==
          expectedTag);

  // The other file-selection switch uses the same recipient route: its
  // disable/re-enable cycle must not allocate, truncate, or rename the
  // existing report file, and suppressed announcements must not consume a
  // serial number.
  REQUIRE(late->getSendServiceReportsToFileSwitch());
  REQUIRE_NOTHROW(late->setSendServiceReportsToFileSwitch(false));
  REQUIRE_FALSE(late->getSendServiceReportsToFileSwitch());
  REQUIRE_NOTHROW(owner->registerFederationSynchronizationPoint(
      L"while-send-file-disabled", tag));
  REQUIRE(readTextFile(lateReportFile) ==
          lateInitialText + lateAnnouncement + reenabledAnnouncement);
  REQUIRE(serviceReportFiles(directory.path()).size() == 2U);
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.size() == 3U);
  REQUIRE_FALSE(late->evokeCallback(0.0));
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.size() == 4U);
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.back().label ==
          L"while-send-file-disabled");
  REQUIRE(variableLengthDataBytes(
              lateReports.synchronizationPointAnnouncementReports.back().userSuppliedTag) ==
          expectedTag);

  REQUIRE_NOTHROW(late->setSendServiceReportsToFileSwitch(true));
  REQUIRE(late->getSendServiceReportsToFileSwitch());
  REQUIRE(readTextFile(lateReportFile) ==
          lateInitialText + lateAnnouncement + reenabledAnnouncement);
  REQUIRE_NOTHROW(owner->registerFederationSynchronizationPoint(
      L"after-send-file-reenabled", tag));
  auto const sendFileReenabledAnnouncement =
      announcementRecord(2U, "after-send-file-reenabled");
  REQUIRE(readTextFile(lateReportFile) ==
          lateInitialText + lateAnnouncement + reenabledAnnouncement +
              sendFileReenabledAnnouncement);
  REQUIRE(serviceReportFiles(directory.path()).size() == 2U);
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.size() == 4U);
  REQUIRE_FALSE(late->evokeCallback(0.0));
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.size() == 5U);
  REQUIRE(lateReports.synchronizationPointAnnouncementReports.back().label ==
          L"after-send-file-reenabled");
  REQUIRE(variableLengthDataBytes(
              lateReports.synchronizationPointAnnouncementReports.back().userSuppliedTag) ==
          expectedTag);

  REQUIRE_NOTHROW(late->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(late->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
