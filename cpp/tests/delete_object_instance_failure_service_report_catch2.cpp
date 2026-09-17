#include <catch2/catch_test_macros.hpp>

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
#error "The receive-order Delete Object Instance service-report failure test requires the Umbra source directory."
#endif

namespace {

using rti1516_2025::AttributeHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectInstanceHandle;
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
        ("umbra-delete-failure-report-" + std::to_string(++counter) + "-" +
         std::to_string(attempt));
    std::error_code error;
    if (std::filesystem::create_directory(path, error)) {
      return ScopedTemporaryDirectory(path);
    }
    if (error && error != std::errc::file_exists) {
      throw std::filesystem::filesystem_error(
          "Unable to reserve temporary service-report directory", path, error);
    }
  }
  throw std::runtime_error(
      "Unable to reserve a unique temporary service-report directory.");
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

std::string ascii(std::wstring const& value) {
  std::string result;
  result.reserve(value.size());
  for (wchar_t const character : value) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    result.push_back(static_cast<char>(character));
  }
  return result;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"delete-object-instance-failure-report-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::string reportRecord(
    std::uint32_t serial,
    std::string const& objectValue,
    bool success,
    std::string const& exception) {
  return std::string{"{\"HLAserialNumber\":"} + std::to_string(serial) +
      R"(,"HLAreturnedArgument":[null],"HLAservice":"DeleteObjectInstance","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectValue +
      R"("},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"cmVn"},{"HLAargumentType":34,"HLAargumentName":"Optional timestamp","HLAargumentValue":null}],"HLAsuccessIndicator":)" +
      (success ? "true" : "false") + R"(,"HLAexception":)" +
      (exception.empty() ? "null" : "\"" + exception + "\"") + "}";
}

}  // namespace

TEST_CASE(
    "Embedded service reporting records failed receive-order Delete Object Instance invocations",
    "[integration][development-profile][federation-management][object-management]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[delete-object-instance-failure-file-matrix]"
    "[rti.service.delete-failure-matrix]") {
  rti1516_2025::NullFederateAmbassador publisherReports;
  rti1516_2025::NullFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
       "ieee1516.2-2025" / "resources" / "examples" /
       "RestaurantFOMmodule-2025.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");
  unsigned char const tagBytes[] = {'r', 'e', 'g'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"delete-failure-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"delete-failure-receiver", L"subscriber", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));

  auto const server = publisher->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const efficiency = publisher->getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet const attributes{efficiency};
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(server, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(server, attributes));
  auto const objectInstance = publisher->registerObjectInstance(server);
  REQUIRE_FALSE(receiver->evokeMultipleCallbacks(0.0, 0.0));

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  REQUIRE(readTextFile(reportFile) == initialText);

  auto const invalidObjectValue = ascii(ObjectInstanceHandle{}.toString());
  auto const validObjectValue = ascii(objectInstance.toString());
  auto const invalidObjectFailure = reportRecord(
      0U,
      invalidObjectValue,
      false,
      "ObjectInstanceNotKnown: The supplied ObjectInstanceHandle is not known to this federate.");
  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(ObjectInstanceHandle{}, tag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText + invalidObjectFailure);

  auto const acceptedRecord = reportRecord(1U, validObjectValue, true, "");
  REQUIRE_NOTHROW(publisher->deleteObjectInstance(objectInstance, tag));
  REQUIRE(readTextFile(reportFile) ==
          initialText + invalidObjectFailure + acceptedRecord);

  auto const deletedObjectFailure = reportRecord(
      2U,
      validObjectValue,
      false,
      "ObjectInstanceNotKnown: The supplied ObjectInstanceHandle is not known to this federate.");
  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(objectInstance, tag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) ==
          initialText + invalidObjectFailure + acceptedRecord +
              deletedObjectFailure);

  REQUIRE_FALSE(receiver->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}
