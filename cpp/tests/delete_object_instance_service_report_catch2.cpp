#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include <umbra/embedded_profile_configuration.hpp>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The receive-order Delete Object Instance service-report test requires the Umbra source directory."
#endif

namespace {

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
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
        ("umbra-delete-report-" + std::to_string(++counter) + "-" +
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

std::vector<unsigned char> bytes(VariableLengthData const& value) {
  auto const* data = static_cast<unsigned char const*>(value.data());
  if (data == nullptr || value.size() == 0U) {
    return {};
  }
  return {data, data + value.size()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"delete-object-instance-service-report-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path reportFileFor(
    std::filesystem::path const& directory,
    std::string const& federateName) {
  auto const marker = std::string{"\"HLAfederateName\":\""} +
      federateName + "\"";
  for (auto const& candidate : serviceReportFiles(directory)) {
    if (readTextFile(candidate).find(marker) != std::string::npos) {
      return candidate;
    }
  }
  FAIL("joined federate service-report file was not found");
  return {};
}

std::string expectedDeleteRecord(ObjectInstanceHandle const& objectInstance) {
  std::string result =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"DeleteObjectInstance","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")";
  result += ascii(objectInstance.toString());
  result +=
      R"("},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"cmVn"},{"HLAargumentType":34,"HLAargumentName":"Optional timestamp","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})";
  return result;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ObjectRemovalReport final {
    ObjectInstanceHandle objectInstance;
    VariableLengthData userSuppliedTag;
    FederateHandle producingFederate;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    static_cast<void>(objectClass);
    static_cast<void>(objectInstanceName);
    static_cast<void>(producingFederate);
    discoveredObjects.push_back(objectInstance);
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& userSuppliedTag,
      FederateHandle const& producingFederate) override {
    if (onObjectRemoval) {
      onObjectRemoval();
    }
    objectRemovalReports.push_back({
        objectInstance,
        userSuppliedTag,
        producingFederate});
  }

  std::vector<ObjectInstanceHandle> discoveredObjects;
  std::vector<ObjectRemovalReport> objectRemovalReports;
  std::function<void()> onObjectRemoval;
};

}  // namespace

TEST_CASE(
    "Embedded service reporting preserves receive-order Delete Object Instance arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[mom][service-report-file][service-reporting]"
    "[delete-object-instance-service-report-file]"
    "[rti.service.delete-object-instance]"
    "[federate.callback.remove-object-instance]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
       "ieee1516.2-2025" / "resources" / "examples" /
       "RestaurantFOMmodule-2025.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto publisherConfiguration = configurationForServiceReportDirectory(directory.path());
  auto receiverConfiguration = configurationForServiceReportDirectory(directory.path());
  publisherConfiguration.withRtiAddress(L"in-process");
  receiverConfiguration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(publisher->connect(
      publisherReports, HLA_EVOKED, publisherConfiguration));
  REQUIRE_NOTHROW(receiver->connect(
      receiverReports, HLA_EVOKED, receiverConfiguration));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, restaurantFom, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"delete-object-report-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"delete-object-report-receiver", L"receiver", federationName));

  auto const publisherReportFile = reportFileFor(
      directory.path(), "delete-object-report-publisher");
  auto const receiverReportFile = reportFileFor(
      directory.path(), "delete-object-report-receiver");
  REQUIRE(publisherReportFile.is_absolute());
  REQUIRE(receiverReportFile.is_absolute());
  REQUIRE(publisherReportFile != receiverReportFile);

  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(receiver->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->setServiceReportingSwitch(false));

  auto const server = publisher->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const efficiency = publisher->getAttributeHandle(server, L"Efficiency");
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  AttributeHandleSet const attributes{efficiency};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(server, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(server, attributes));

  ObjectInstanceHandle const objectInstance =
      publisher->registerObjectInstance(server);
  REQUIRE_FALSE(receiver->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(receiverReports.discoveredObjects.size() == 1U);

  auto const publisherBeforeDelete = readTextFile(publisherReportFile);
  auto const receiverBeforeDelete = readTextFile(receiverReportFile);
  REQUIRE_FALSE(publisherBeforeDelete.empty());
  REQUIRE_FALSE(receiverBeforeDelete.empty());

  unsigned char const tagBytes[] = {'r', 'e', 'g'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(unknownObject, tag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(publisherReportFile) == publisherBeforeDelete);
  REQUIRE(readTextFile(receiverReportFile) == receiverBeforeDelete);

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  REQUIRE(publisher->getServiceReportingSwitch());
  REQUIRE(publisher->getSendServiceReportsToFileSwitch());
  REQUIRE(readTextFile(publisherReportFile) == publisherBeforeDelete);

  auto const expectedRecord = expectedDeleteRecord(objectInstance);

  std::vector<std::string> publisherFileAtRemovalEntry;
  receiverReports.onObjectRemoval = [&] {
    publisherFileAtRemovalEntry.push_back(readTextFile(publisherReportFile));
  };
  REQUIRE_NOTHROW(publisher->deleteObjectInstance(objectInstance, tag));
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(publisherFileAtRemovalEntry.empty());
  REQUIRE(readTextFile(publisherReportFile) ==
          publisherBeforeDelete + expectedRecord);
  REQUIRE(readTextFile(receiverReportFile) == receiverBeforeDelete);

  REQUIRE_FALSE(receiver->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(receiverReports.objectRemovalReports.size() == 1U);
  REQUIRE(publisherFileAtRemovalEntry.size() == 1U);
  REQUIRE(publisherFileAtRemovalEntry.front() ==
          publisherBeforeDelete + expectedRecord);
  REQUIRE(readTextFile(publisherReportFile) ==
          publisherBeforeDelete + expectedRecord);
  REQUIRE(readTextFile(receiverReportFile) == receiverBeforeDelete);

  auto const& removal = receiverReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(bytes(removal.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(removal.producingFederate == publisherHandle);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
