#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"

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
#error "The class Request Attribute Value Update service-report test requires the Umbra source directory."
#endif

namespace {

namespace fixture_hla = umbra::test::hla::wide;

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
        ("umbra-object-class-service-report-" +
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
  return L"object-class-attribute-value-update-service-report-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ProvideReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
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

  void provideAttributeValueUpdate(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    if (onProvideAttributeValueUpdate) {
      onProvideAttributeValueUpdate();
    }
    provideReports.push_back({objectInstance, attributes, userSuppliedTag});
  }

  std::vector<ObjectInstanceHandle> discoveredObjects;
  std::vector<ProvideReport> provideReports;
  std::function<void()> onProvideAttributeValueUpdate;
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

std::string expectedProvideRecord(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandle const& attribute,
    std::uint32_t serialNumber) {
  return std::string{
      R"({"HLAserialNumber":)"} + std::to_string(serialNumber) +
      R"(,"HLAreturnedArgument":[null],"HLAservice":"ProvideAttributeValueUpdate","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      ascii(objectInstance.toString()) +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      ascii(attribute.toString()) +
      R"("]},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"dHNv"}],"HLAsuccessIndicator":true,"HLAexception":null})";
}

}  // namespace

TEST_CASE(
    "Embedded class Request Attribute Value Update reports each provider callback before delivery",
    "[integration][development-profile][federation-management][object-management][callbacks]"
    "[mom][service-report-file][service-reporting]"
    "[object-class-provide-attribute-value-update-service-report]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[rti.service.request-attribute-value-update]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "attribute-update-passel-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto ownerConfiguration = configurationForServiceReportDirectory(directory.path());
  auto requesterConfiguration = configurationForServiceReportDirectory(directory.path());
  ownerConfiguration.withRtiAddress(L"in-process");
  requesterConfiguration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED, ownerConfiguration));
  REQUIRE_NOTHROW(
      requester->connect(requesterReports, HLA_EVOKED, requesterConfiguration));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"object-class-provide-report-owner", L"provider", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"object-class-provide-report-requester", L"requester", federationName));

  auto const ownerReportFile = reportFileFor(
      directory.path(), "object-class-provide-report-owner");
  auto const requesterReportFile = reportFileFor(
      directory.path(), "object-class-provide-report-requester");
  REQUIRE(ownerReportFile.is_absolute());
  REQUIRE(requesterReportFile.is_absolute());
  REQUIRE(ownerReportFile != requesterReportFile);

  // Keep setup records out of the provider callback proof.  The selected
  // filesystem file remains the joined-federate identity, beginning with its
  // initial record and receiving only the callback records below.
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(false));

  auto const baseClass = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_base);
  auto const childClass = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliableBaseA = owner->getAttributeHandle(
      baseClass, fixture_hla::fixture::reliable_base_a);
  auto const reliableBaseB = owner->getAttributeHandle(
      baseClass, fixture_hla::fixture::reliable_base_b);
  auto const reliableChild = owner->getAttributeHandle(
      childClass, fixture_hla::fixture::reliable_child);
  REQUIRE(baseClass.isValid());
  REQUIRE(childClass.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableBaseB.isValid());
  REQUIRE(reliableChild.isValid());

  AttributeHandleSet const ownerAttributes{reliableBaseA, reliableChild};
  AttributeHandleSet const requesterAttributes{reliableBaseB};
  AttributeHandleSet const requestAttributes{reliableBaseA, reliableBaseB};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      childClass, ownerAttributes));
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      baseClass, requesterAttributes));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
      baseClass, requestAttributes));

  std::vector<ObjectInstanceHandle> ownerObjects;
  REQUIRE_NOTHROW(ownerObjects.push_back(owner->registerObjectInstance(childClass)));
  REQUIRE_NOTHROW(ownerObjects.push_back(owner->registerObjectInstance(childClass)));
  REQUIRE_NOTHROW(requester->registerObjectInstance(baseClass));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.discoveredObjects.size() >= ownerObjects.size());

  auto const ownerBeforeRequest = readTextFile(ownerReportFile);
  auto const requesterBeforeRequest = readTextFile(requesterReportFile);
  REQUIRE_FALSE(ownerBeforeRequest.empty());
  REQUIRE_FALSE(requesterBeforeRequest.empty());
  REQUIRE(ownerBeforeRequest.find("HLAfederateName") != std::string::npos);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE(owner->getServiceReportingSwitch());
  REQUIRE(owner->getSendServiceReportsToFileSwitch());
  REQUIRE(readTextFile(ownerReportFile) == ownerBeforeRequest);

  std::vector<std::string> reportAtCallbackEntry;
  ownerReports.onProvideAttributeValueUpdate = [&] {
    reportAtCallbackEntry.push_back(readTextFile(ownerReportFile));
  };
  std::vector<unsigned char> const requestTagBytes{
      0x74U, 0x73U, 0x6FU};  // "tso" -> dHNv in the Table 5 file form.
  VariableLengthData const requestTag(
      requestTagBytes.data(), requestTagBytes.size());
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdate(
      baseClass, requestAttributes, requestTag));
  REQUIRE(reportAtCallbackEntry.empty());
  REQUIRE(readTextFile(ownerReportFile) == ownerBeforeRequest);

  drainCallbacks(*owner);
  REQUIRE(ownerReports.provideReports.size() == ownerObjects.size());
  REQUIRE(reportAtCallbackEntry.size() == ownerObjects.size());
  std::string expectedFile = ownerBeforeRequest;
  for (std::size_t index = 0U; index != ownerObjects.size(); ++index) {
    auto const& provide = ownerReports.provideReports[index];
    REQUIRE(provide.objectInstance == ownerObjects[index]);
    REQUIRE(provide.attributes == AttributeHandleSet{reliableBaseA});
    REQUIRE(provide.userSuppliedTag.size() == requestTagBytes.size());
    REQUIRE(reportAtCallbackEntry[index] ==
            expectedFile + expectedProvideRecord(
                ownerObjects[index], reliableBaseA,
                static_cast<std::uint32_t>(index)));
    expectedFile += expectedProvideRecord(
        ownerObjects[index], reliableBaseA,
        static_cast<std::uint32_t>(index));
  }
  REQUIRE(readTextFile(ownerReportFile) == expectedFile);
  REQUIRE(readTextFile(requesterReportFile) == requesterBeforeRequest);
  REQUIRE(requesterReports.provideReports.empty());

  REQUIRE_NOTHROW(requester->unsubscribeObjectClassAttributes(
      baseClass, requestAttributes));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      baseClass, requesterAttributes));
  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
      childClass, ownerAttributes));
  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
