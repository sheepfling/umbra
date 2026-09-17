#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
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
#include <string_view>
#include <system_error>
#include <vector>

#include <umbra/embedded_profile_configuration.hpp>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The Auto Provide service-report test requires the Umbra source directory."
#endif

namespace {

namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::CallbackModel;
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
        ("umbra-auto-provide-service-report-" +
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
  return L"auto-provide-service-report-" +
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

}  // namespace

TEST_CASE(
    "Embedded Auto Provide service reporting records empty tag before its callback",
    "[integration][development-profile][federation-management][object-management][callbacks]"
    "[mom][service-report-file][service-reporting][auto-provide]"
    "[auto-provide-service-report]"
    "[rti.service.get-auto-provide-switch]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
       "ieee1516.2-2025" / "resources" / "examples" /
       "RestaurantFOMmodule-2025.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom};
  auto directory = temporaryServiceReportDirectory();
  auto ownerConfiguration = configurationForServiceReportDirectory(directory.path());
  auto requesterConfiguration = configurationForServiceReportDirectory(directory.path());
  ownerConfiguration.withRtiAddress(L"in-process");
  requesterConfiguration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED, ownerConfiguration));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED, requesterConfiguration));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"auto-provide-report-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"auto-provide-report-requester", L"subscriber", federationName));

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
  auto const ownerReportFile = reportFileFor("auto-provide-report-owner");
  auto const requesterReportFile = reportFileFor("auto-provide-report-requester");
  REQUIRE(ownerReportFile.is_absolute());
  REQUIRE(requesterReportFile.is_absolute());
  REQUIRE(ownerReportFile != requesterReportFile);

  REQUIRE(owner->getAutoProvideSwitch());
  REQUIRE(requester->getAutoProvideSwitch());
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(false));

  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = owner->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  AttributeHandleSet const attributes{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, attributes));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(soda, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(soda));
  REQUIRE(objectInstance.isValid());
  REQUIRE(requesterReports.discoveredObjects.empty());
  REQUIRE(ownerReports.provideReports.empty());
  auto const ownerBeforeAutoProvide = readTextFile(ownerReportFile);
  auto const requesterBeforeAutoProvide = readTextFile(requesterReportFile);
  REQUIRE(readTextFile(ownerReportFile) == ownerBeforeAutoProvide);
  REQUIRE(readTextFile(requesterReportFile) == requesterBeforeAutoProvide);

  // Registration is performed while file reporting is disabled, so its own
  // successful service record cannot consume the serial-0 slot.  Enabling the
  // provider's file route before the queued discovery callback runs causes
  // only the subsequent RTI-invoked Auto Provide callback to be recorded.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE(owner->getServiceReportingSwitch());
  REQUIRE(owner->getSendServiceReportsToFileSwitch());
  auto const ownerBeforeDiscovery = readTextFile(ownerReportFile);
  REQUIRE(ownerBeforeDiscovery == ownerBeforeAutoProvide);

  // Discovery is queued on the subscriber separately from the provider's
  // Auto Provide callback.  Neither queue may mutate the provider file until
  // the callback's actual delivery boundary.
  REQUIRE_NOTHROW(requester->evokeCallback(0.0));
  REQUIRE(requesterReports.discoveredObjects.size() == 1U);
  REQUIRE(ownerReports.provideReports.empty());
  REQUIRE(readTextFile(ownerReportFile) == ownerBeforeDiscovery);

  auto const expectedRecord =
      std::string{
          R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"ProvideAttributeValueUpdate","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")"} +
      ascii(objectInstance.toString()) +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      ascii(flavor.toString()) +
      R"("]},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":""}],"HLAsuccessIndicator":true,"HLAexception":null})";
  bool reportPresentAtCallbackEntry = false;
  std::string reportAtCallbackEntry;
  ownerReports.onProvideAttributeValueUpdate = [&] {
    reportAtCallbackEntry = readTextFile(ownerReportFile);
    reportPresentAtCallbackEntry =
        reportAtCallbackEntry == ownerBeforeDiscovery + expectedRecord;
  };

  for (int pass = 0; pass != 32 && ownerReports.provideReports.empty(); ++pass) {
    REQUIRE_NOTHROW(owner->evokeCallback(0.0));
  }
  REQUIRE(reportPresentAtCallbackEntry);
  REQUIRE(ownerReports.provideReports.size() == 1U);
  auto const& provide = ownerReports.provideReports.front();
  REQUIRE(provide.objectInstance == objectInstance);
  REQUIRE(provide.attributes == attributes);
  REQUIRE(provide.userSuppliedTag.size() == 0U);
  REQUIRE(readTextFile(ownerReportFile) == ownerBeforeDiscovery + expectedRecord);

  REQUIRE_NOTHROW(requester->unsubscribeObjectClassAttributes(soda, attributes));
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
