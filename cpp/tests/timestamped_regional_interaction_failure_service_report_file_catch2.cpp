#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>
#include <umbra/embedded_profile_configuration.hpp>

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

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped regional interaction failure service-report test requires the Umbra source directory."
#endif

namespace {

namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RangeBounds;
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
  static std::atomic_uint64_t sequence{0U};
  auto const parent = std::filesystem::temp_directory_path();
  for (std::size_t attempt = 0U; attempt != 1024U; ++attempt) {
    auto const path = parent /
        ("umbra-timestamped-regional-failure-report-" +
         std::to_string(sequence.fetch_add(1U, std::memory_order_relaxed)) +
         "-" + std::to_string(attempt));
    std::error_code error;
    if (std::filesystem::create_directory(path, error)) {
      return ScopedTemporaryDirectory(path);
    }
    if (error && error != std::errc::file_exists) {
      throw std::filesystem::filesystem_error(
          "Unable to reserve a temporary service-report directory", path, error);
    }
  }
  throw std::runtime_error("Unable to reserve a unique service-report directory.");
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
  return L"timestamped-regional-failure-report-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::string asAscii(std::wstring const& text) {
  std::string result;
  result.reserve(text.size());
  for (wchar_t const character : text) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    result.push_back(static_cast<char>(character));
  }
  return result;
}

std::string failureRecord(
    std::uint32_t serial,
    std::string const& interactionValue,
    std::string const& parameterMapValue,
    std::string const& regionSetValue,
    std::string const& timestampValue,
    std::string const& exception) {
  return std::string{"{\"HLAserialNumber\":"} + std::to_string(serial) +
      R"(,"HLAreturnedArgument":[null],"HLAservice":"SendInteractionWithRegions","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionValue +
      R"("},{"HLAargumentType":40,"HLAargumentName":"Constrained set of interaction parameter designator and value pairs","HLAargumentValue":)" +
      parameterMapValue +
      R"(},{"HLAargumentType":43,"HLAargumentName":"Set of region designators","HLAargumentValue":)" +
      regionSetValue +
      R"(},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"dHNv"},{"HLAargumentType":31,"HLAargumentName":"Optional timestamp","HLAargumentValue":)" +
      timestampValue +
      R"(}],"HLAsuccessIndicator":false,"HLAexception":")" +
      exception + "\"}";
}

}  // namespace

TEST_CASE(
    "Embedded service reporting records failed timestamped regional Send Interaction With Regions invocations",
    "[integration][development-profile][federation-management][interaction-management]"
    "[ddm][time-management][mom][service-report-file][service-reporting][service-failure][tso]"
    "[timestamped-regional-interaction-failure][timestamped-regional-interaction-failure-service-report-file]"
    "[rti.service.send-interaction-with-regions][rti.service.change-interaction-order-type]"
    "[rti.service.enable-time-regulation][rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]") {
  rti1516_2025::NullFederateAmbassador publisherCallbacks;
  auto publisher = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");
  unsigned char const parameterBytes[] = {0x01, 0x02};
  unsigned char const tagBytes[] = {'t', 's', 'o'};
  VariableLengthData const parameterValue(parameterBytes, sizeof(parameterBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-regional-failure-publisher",
      L"publisher",
      federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const parameter = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::temperature_ok);
  auto const dimension = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE(dimension.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(
      interactionClass,
      rti1516_2025::TIMESTAMP));
  auto const region = publisher->createRegion(
      rti1516_2025::DimensionHandleSet{dimension});
  REQUIRE_NOTHROW(publisher->setRangeBounds(region, dimension, RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{region}));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  auto const baseline = readTextFile(reportFile);
  REQUIRE(baseline == initialText);

  auto const invalidInteractionValue = asAscii(InteractionClassHandle{}.toString());
  auto const validInteractionValue = asAscii(interactionClass.toString());
  auto const parameterMapValue =
      std::string{"{\""} + asAscii(parameter.toString()) + R"(":"AQI="})";
  auto const invalidParameterMapValue =
      std::string{"{\""} + asAscii(ParameterHandle{}.toString()) + R"(":"AQI="})";
  auto const regionSetValue =
      std::string{"[\""} + asAscii(region.toString()) + "\"]";
  auto const invalidRegionSetValue =
      std::string{"[\""} + asAscii(RegionHandle{}.toString()) + "\"]";

  auto const invalidInteractionFailure = failureRecord(
      0U,
      invalidInteractionValue,
      parameterMapValue,
      regionSetValue,
      "\"6\"",
      "InteractionClassNotDefined: Timestamped Send Interaction With Regions requires a defined InteractionClassHandle.");
  REQUIRE_THROWS_AS(
      publisher->sendInteractionWithRegions(
          InteractionClassHandle{},
          ParameterHandleValueMap{{parameter, parameterValue}},
          RegionHandleSet{region},
          tag,
          rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE(readTextFile(reportFile) == baseline + invalidInteractionFailure);

  auto const invalidParameterFailure = failureRecord(
      1U,
      validInteractionValue,
      invalidParameterMapValue,
      regionSetValue,
      "\"6\"",
      "InteractionParameterNotDefined: Timestamped Send Interaction With Regions requires defined ParameterHandle values.");
  REQUIRE_THROWS_AS(
      publisher->sendInteractionWithRegions(
          interactionClass,
          ParameterHandleValueMap{{ParameterHandle{}, parameterValue}},
          RegionHandleSet{region},
          tag,
          rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::InteractionParameterNotDefined);
  REQUIRE(readTextFile(reportFile) == baseline + invalidInteractionFailure +
          invalidParameterFailure);

  auto const invalidRegionFailure = failureRecord(
      2U,
      validInteractionValue,
      parameterMapValue,
      invalidRegionSetValue,
      "\"6\"",
      "InvalidRegion: Timestamped Send Interaction With Regions requires valid RegionHandle values.");
  REQUIRE_THROWS_AS(
      publisher->sendInteractionWithRegions(
          interactionClass,
          ParameterHandleValueMap{{parameter, parameterValue}},
          RegionHandleSet{RegionHandle{}},
          tag,
          rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::InvalidRegion);
  REQUIRE(readTextFile(reportFile) == baseline + invalidInteractionFailure +
          invalidParameterFailure + invalidRegionFailure);

  auto const invalidTimestampFailure = failureRecord(
      3U,
      validInteractionValue,
      parameterMapValue,
      regionSetValue,
      "\"4\"",
      "InvalidLogicalTime: A timestamped service is earlier than the sender\\'s current logical time plus lookahead.");
  REQUIRE_THROWS_AS(
      publisher->sendInteractionWithRegions(
          interactionClass,
          ParameterHandleValueMap{{parameter, parameterValue}},
          RegionHandleSet{region},
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE(readTextFile(reportFile) == baseline + invalidInteractionFailure +
          invalidParameterFailure + invalidRegionFailure + invalidTimestampFailure);

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(publisher->deleteRegion(region));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
}
