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
        ("umbra-sync-confirm-service-report-" +
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
  return L"service-report-confirm-synchronization-point-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct SynchronizationPointRegistrationReport final {
    std::wstring label;
    bool succeeded = false;
    rti1516_2025::SynchronizationPointFailureReason failureReason =
        rti1516_2025::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE;
  };

  void synchronizationPointRegistrationSucceeded(
      std::wstring const& label) override {
    synchronizationPointRegistrationReports.push_back({label, true});
  }

  void synchronizationPointRegistrationFailed(
      std::wstring const& label,
      rti1516_2025::SynchronizationPointFailureReason reason) override {
    synchronizationPointRegistrationReports.push_back({label, false, reason});
  }

  std::vector<SynchronizationPointRegistrationReport>
      synchronizationPointRegistrationReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded service reporting records Confirm Synchronization Point Registration result forms",
    "[integration][development-profile][federation-management][synchronization]"
    "[mom][service-report-file][service-reporting]"
    "[confirm-synchronization-point-registration-service-report]"
    "[rti.service.register-federation-synchronization-point]"
    "[federate.callback.synchronization-point-registration-succeeded]"
    "[federate.callback.synchronization-point-registration-failed]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
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
  unsigned char const tagBytes[] = {0x43, 0x4f, 0x4e, 0x46};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"sync-confirm-report", L"sync", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Both public callback selectors represent one §4.15 service. On success,
  // its third optional argument is explicitly Null; on a duplicate label it
  // carries the Table 5 type-56 reason. The §4.16 announcement of the
  // successful label is a separate RTI-initiated record. All records must be
  // durable before their HLA_EVOKED callbacks are dispatched.
  REQUIRE_NOTHROW(rti->registerFederationSynchronizationPoint(
      L"confirm-report", tag));
  auto const registerSuccess =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"RegisterFederationSynchronizationPoint","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Synchronization point label","HLAargumentValue":"confirm-report"},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"Q09ORg=="},{"HLAargumentType":34,"HLAargumentName":"Optional set of joined federate designators","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const confirmSuccess =
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"ConfirmSynchronizationPointRegistration","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Synchronization point label","HLAargumentValue":"confirm-report"},{"HLAargumentType":6,"HLAargumentName":"Registration-success indicator","HLAargumentValue":true},{"HLAargumentType":34,"HLAargumentName":"Optional failure reason","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const announceSuccess =
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"AnnounceSynchronizationPoint","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Synchronization point label","HLAargumentValue":"confirm-report"},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"Q09ORg=="}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) ==
          initialText + registerSuccess + confirmSuccess + announceSuccess);
  REQUIRE(reports.synchronizationPointRegistrationReports.empty());
  REQUIRE(rti->evokeCallback(0.0));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.synchronizationPointRegistrationReports.size() == 1U);
  REQUIRE(reports.synchronizationPointRegistrationReports.front().succeeded);

  REQUIRE_NOTHROW(rti->registerFederationSynchronizationPoint(
      L"confirm-report", tag));
  auto const registerFailure =
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[null],"HLAservice":"RegisterFederationSynchronizationPoint","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Synchronization point label","HLAargumentValue":"confirm-report"},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"Q09ORg=="},{"HLAargumentType":34,"HLAargumentName":"Optional set of joined federate designators","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const confirmFailure =
      R"({"HLAserialNumber":4,"HLAreturnedArgument":[null],"HLAservice":"ConfirmSynchronizationPointRegistration","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Synchronization point label","HLAargumentValue":"confirm-report"},{"HLAargumentType":6,"HLAargumentName":"Registration-success indicator","HLAargumentValue":false},{"HLAargumentType":56,"HLAargumentName":"Optional failure reason","HLAargumentValue":"SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) ==
          initialText + registerSuccess + confirmSuccess + announceSuccess +
              registerFailure + confirmFailure);
  REQUIRE(reports.synchronizationPointRegistrationReports.size() == 1U);
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.synchronizationPointRegistrationReports.size() == 2U);
  REQUIRE_FALSE(reports.synchronizationPointRegistrationReports.back().succeeded);
  REQUIRE(reports.synchronizationPointRegistrationReports.back().failureReason ==
          rti1516_2025::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE);

  REQUIRE_NOTHROW(rti->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting preserves Register Federation Synchronization Point arguments",
    "[integration][development-profile][federation-management][synchronization]"
    "[mom][service-report-file][service-reporting]"
    "[register-federation-synchronization-point][rti.service.register-federation-synchronization-point]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
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
  unsigned char const tagBytes[] = {0x52, 0x45, 0x47, 0x53};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  auto const joined = rti->joinFederationExecution(
      L"sync-register-report", L"sync", federationName);
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // The two-argument overload must preserve the omitted optional set as the
  // Table 5 type-34 Null form, rather than inventing an empty array.
  REQUIRE_NOTHROW(rti->registerFederationSynchronizationPoint(
      L"register-default", tag));
  auto const afterDefault = readTextFile(reportFile);
  REQUIRE(afterDefault.size() > initialText.size());
  REQUIRE(afterDefault.find(
              R"("HLAservice":"RegisterFederationSynchronizationPoint")") !=
          std::string::npos);
  REQUIRE(afterDefault.find(
              R"("HLAargumentType":34,"HLAargumentName":"Optional set of joined federate designators","HLAargumentValue":null)") !=
          std::string::npos);

  // The explicit overload must preserve the supplied FederateHandleSet as the
  // Table 5 type-18 array form while keeping the same logical file identity.
  rti1516_2025::FederateHandleSet explicitSet;
  explicitSet.insert(joined);
  REQUIRE_NOTHROW(rti->registerFederationSynchronizationPoint(
      L"register-explicit", tag, explicitSet));
  auto const afterExplicit = readTextFile(reportFile);
  REQUIRE(afterExplicit.size() > afterDefault.size());
  REQUIRE(afterExplicit.find(
              R"("HLAargumentType":18,"HLAargumentName":"Optional set of joined federate designators","HLAargumentValue":[)") !=
          std::string::npos);
  REQUIRE(afterExplicit.find("register-explicit") !=
          std::string::npos);
  REQUIRE(serviceReportFiles(directory.path()).size() == 1U);

  REQUIRE_NOTHROW(rti->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
