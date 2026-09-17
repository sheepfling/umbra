#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
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
#error "The receive-order Update Attribute Values service-report test requires the Umbra source directory."
#endif

namespace {

namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
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
        ("umbra-update-attribute-service-report-" +
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
  return L"update-attribute-values-service-report-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
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

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    if (onAttributeReflection) {
      onAttributeReflection();
    }
    reflections.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr});
  }

  std::vector<ObjectInstanceHandle> discoveredObjects;
  std::vector<ReflectionReport> reflections;
  std::function<void()> onAttributeReflection;
};

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 64; ++pass) {
    static_cast<void>(rti.evokeCallback(0.0));
  }
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

std::string expectedUpdateRecord(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandle const& attribute,
    std::uint32_t serialNumber,
    std::string const& tagValue) {
  return std::string{
      R"({"HLAserialNumber":)"} + std::to_string(serialNumber) +
      R"(,"HLAreturnedArgument":[null],"HLAservice":"UpdateAttributeValues","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      ascii(objectInstance.toString()) +
      R"("},{"HLAargumentType":2,"HLAargumentName":"Constrained set of attribute designator and value pairs","HLAargumentValue":{")" +
      ascii(attribute.toString()) +
      R"(":"AQI="}},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":")" +
      tagValue +
      R"("},{"HLAargumentType":34,"HLAargumentName":"Optional timestamp","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})";
}

}  // namespace

TEST_CASE(
    "Embedded service reporting preserves receive-order Update Attribute Values arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[mom][service-report-file][service-reporting]"
    "[update-attribute-values-service-report-file]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "attribute-update-passel-fom.xml")
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
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"update-attribute-service-report-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"update-attribute-service-report-receiver", L"receiver", federationName));

  auto const publisherReportFile = reportFileFor(
      directory.path(), "update-attribute-service-report-publisher");
  auto const receiverReportFile = reportFileFor(
      directory.path(), "update-attribute-service-report-receiver");
  REQUIRE(publisherReportFile.is_absolute());
  REQUIRE(receiverReportFile.is_absolute());
  REQUIRE(publisherReportFile != receiverReportFile);

  // Suppress setup records, retaining each joined federate's initial record as
  // the stable boundary for the accepted receive-order update.
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(receiver->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->setServiceReportingSwitch(false));

  auto const baseClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_base);
  auto const childClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliableBaseA = publisher->getAttributeHandle(
      baseClass, fixture_hla::fixture::reliable_base_a);
  auto const reliableBaseB = publisher->getAttributeHandle(
      baseClass, fixture_hla::fixture::reliable_base_b);
  auto const reliableChild = publisher->getAttributeHandle(
      childClass, fixture_hla::fixture::reliable_child);
  REQUIRE(baseClass.isValid());
  REQUIRE(childClass.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableBaseB.isValid());
  REQUIRE(reliableChild.isValid());

  AttributeHandleSet const publisherAttributes{reliableBaseA, reliableChild};
  AttributeHandleSet const receiverAttributes{reliableBaseB};
  AttributeHandleSet const subscriptionAttributes{reliableBaseA, reliableBaseB};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      childClass, publisherAttributes));
  REQUIRE_NOTHROW(receiver->publishObjectClassAttributes(
      baseClass, receiverAttributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      baseClass, subscriptionAttributes));

  auto const objectInstance = publisher->registerObjectInstance(childClass);
  REQUIRE_NOTHROW(receiver->registerObjectInstance(baseClass));
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.discoveredObjects.size() >= 1U);

  auto const publisherBeforeUpdate = readTextFile(publisherReportFile);
  auto const receiverBeforeUpdate = readTextFile(receiverReportFile);
  REQUIRE_FALSE(publisherBeforeUpdate.empty());
  REQUIRE_FALSE(receiverBeforeUpdate.empty());
  REQUIRE(publisherBeforeUpdate.find("HLAfederateName") != std::string::npos);

  std::vector<unsigned char> const valueBytes{0x01U, 0x02U};
  std::vector<unsigned char> const tagBytes{'r', 'e', 'g'};
  VariableLengthData const value(valueBytes.data(), valueBytes.size());
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  AttributeHandleValueMap const values{{reliableBaseA, value}};

  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(unknownObject, values, tag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(publisherReportFile) == publisherBeforeUpdate);
  REQUIRE(readTextFile(receiverReportFile) == receiverBeforeUpdate);

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  REQUIRE(publisher->getServiceReportingSwitch());
  REQUIRE(publisher->getSendServiceReportsToFileSwitch());
  REQUIRE(readTextFile(publisherReportFile) == publisherBeforeUpdate);

  auto const expectedRecord = expectedUpdateRecord(
      objectInstance, reliableBaseA, 0U, "cmVn");
  std::vector<std::string> publisherFileAtReflectionEntry;
  receiverReports.onAttributeReflection = [&] {
    publisherFileAtReflectionEntry.push_back(readTextFile(publisherReportFile));
  };

  REQUIRE_NOTHROW(publisher->updateAttributeValues(objectInstance, values, tag));
  REQUIRE(receiverReports.reflections.empty());
  REQUIRE(publisherFileAtReflectionEntry.empty());
  REQUIRE(readTextFile(publisherReportFile) ==
          publisherBeforeUpdate + expectedRecord);
  REQUIRE(readTextFile(receiverReportFile) == receiverBeforeUpdate);

  drainCallbacks(*receiver);
  REQUIRE(receiverReports.reflections.size() == 1U);
  REQUIRE(publisherFileAtReflectionEntry.size() == 1U);
  REQUIRE(publisherFileAtReflectionEntry.front() ==
          publisherBeforeUpdate + expectedRecord);
  REQUIRE(readTextFile(publisherReportFile) ==
          publisherBeforeUpdate + expectedRecord);
  REQUIRE(readTextFile(receiverReportFile) == receiverBeforeUpdate);

  auto const& reflection = receiverReports.reflections.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(reliableBaseA));
  REQUIRE(bytes(reflection.attributeValues.at(reliableBaseA)) == valueBytes);
  REQUIRE(bytes(reflection.userSuppliedTag) == tagBytes);
  REQUIRE(reflection.producingFederate.isValid());
  REQUIRE(reflection.transportationType.isValid());
  REQUIRE_FALSE(reflection.sentRegionsSupplied);
  REQUIRE(publisherReports.reflections.empty());

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(
      baseClass, subscriptionAttributes));
  REQUIRE_NOTHROW(receiver->unpublishObjectClassAttributes(
      baseClass, receiverAttributes));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      childClass, publisherAttributes));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
