#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

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
#include <utility>
#include <vector>

#include <umbra/embedded_profile_configuration.hpp>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The declaration-relevance service-report test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::CallbackModel;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;

struct ObjectClassRelevanceReport final {
  ObjectClassHandle objectClass;
};

struct InteractionRelevanceReport final {
  InteractionClassHandle interactionClass;
};

class DeclarationReportFederateAmbassador final : public NullFederateAmbassador {
 public:
  void startRegistrationForObjectClass(
      ObjectClassHandle const& objectClass) override {
    startRegistrationForObjectClassReports.push_back({objectClass});
  }

  void stopRegistrationForObjectClass(
      ObjectClassHandle const& objectClass) override {
    stopRegistrationForObjectClassReports.push_back({objectClass});
  }

  void turnInteractionsOn(
      InteractionClassHandle const& interactionClass) override {
    turnInteractionsOnReports.push_back({interactionClass});
  }

  void turnInteractionsOff(
      InteractionClassHandle const& interactionClass) override {
    turnInteractionsOffReports.push_back({interactionClass});
  }

  std::vector<ObjectClassRelevanceReport> startRegistrationForObjectClassReports;
  std::vector<ObjectClassRelevanceReport> stopRegistrationForObjectClassReports;
  std::vector<InteractionRelevanceReport> turnInteractionsOnReports;
  std::vector<InteractionRelevanceReport> turnInteractionsOffReports;
};

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
        ("umbra-declaration-relevance-report-" +
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

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0U};
  return L"umbra-declaration-relevance-report-" +
      std::to_wstring(++counter);
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

}  // namespace

TEST_CASE(
    "Embedded service reporting records declaration relevance advisories before callbacks",
    "[integration][development-profile][federation-management][declaration-management][callbacks]"
    "[mom][service-report-file][service-reporting]"
    "[declaration-relevance-advisory-service-report]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.publish-interaction-class]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class]"
    "[federate.callback.start-registration-for-object-class]"
    "[federate.callback.stop-registration-for-object-class]"
    "[federate.callback.turn-interactions-on]"
    "[federate.callback.turn-interactions-off][2025]") {
  DeclarationReportFederateAmbassador publisherReports;
  DeclarationReportFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto publisherConfiguration = configurationForServiceReportDirectory(directory.path());
  auto subscriberConfiguration = configurationForServiceReportDirectory(directory.path());
  publisherConfiguration.withRtiAddress(L"in-process");
  subscriberConfiguration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(
      publisher->connect(publisherReports, HLA_EVOKED, publisherConfiguration));
  REQUIRE_NOTHROW(
      subscriber->connect(subscriberReports, HLA_EVOKED, subscriberConfiguration));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"declaration-report-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"declaration-report-subscriber", L"subscriber", federationName));

  auto const reportFileFor = [&directory](std::string const& federateName) {
    auto const marker = std::string{"\"HLAfederateName\":\""} + federateName +
        "\"";
    for (auto const& candidate : serviceReportFiles(directory.path())) {
      if (readTextFile(candidate).find(marker) != std::string::npos) {
        return candidate;
      }
    }
    FAIL("joined federate service-report file was not found");
    return std::filesystem::path{};
  };
  auto const publisherReportFile = reportFileFor("declaration-report-publisher");
  auto const publisherInitialText = readTextFile(publisherReportFile);

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const advisoryRecord = [](std::uint32_t serialNumber,
                                 std::string const& service,
                                 std::uint16_t argumentType,
                                 std::string const& argumentName,
                                 std::string const& argumentValue) {
    return std::string{R"({"HLAserialNumber":)"} + std::to_string(serialNumber) +
        R"(,"HLAreturnedArgument":[null],"HLAservice":")" + service +
        R"(","HLAsuppliedArguments":[{"HLAargumentType":)" +
        std::to_string(argumentType) + R"(,"HLAargumentName":")" + argumentName +
        R"(","HLAargumentValue":")" + argumentValue +
        R"("}],"HLAsuccessIndicator":true,"HLAexception":null})";
  };

  auto const employee =
      publisher->getObjectClassHandle(fixture_hla::fom::employee);
  auto const name = publisher->getAttributeHandle(
      employee, fixture_hla::fixture::name);
  auto const takeOrder = publisher->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  REQUIRE(employee.isValid());
  REQUIRE(name.isValid());
  REQUIRE(takeOrder.isValid());
  auto const employeeValue = asAscii(employee.toString());
  auto const takeOrderValue = asAscii(takeOrder.toString());
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));

  // §5.14 is RTI-initiated at the publisher after its accepted declaration.
  // The publisher's own §5.2 report takes serial zero; the advisory must take
  // serial one and be durable before its HLA_EVOKED callback can be delivered.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}, true));
  REQUIRE(readTextFile(publisherReportFile) == publisherInitialText);
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  auto const startRecord = advisoryRecord(
      1U,
      "StartRegistrationForObjectClass",
      36U,
      "Object class designator",
      employeeValue);
  auto const afterStart = readTextFile(publisherReportFile);
  REQUIRE(afterStart.ends_with(startRecord));
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.empty());
  while (publisher->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.size() == 1U);
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.front().objectClass ==
          employee);

  // §5.15 has no direct publisher invocation here, so its callback report is
  // exactly the next record after the prior §5.14 report.
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  auto const stopRecord = advisoryRecord(
      2U,
      "StopRegistrationForObjectClass",
      36U,
      "Object class designator",
      employeeValue);
  auto const afterStop = readTextFile(publisherReportFile);
  REQUIRE(afterStop == afterStart + stopRecord);
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.empty());
  while (publisher->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.size() == 1U);
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.front().objectClass ==
          employee);

  // The interaction counterpart has the publisher's §5.4 report at serial
  // three and the RTI-initiated §5.16 report at serial four.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, true));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(takeOrder));
  auto const turnOnRecord = advisoryRecord(
      4U,
      "TurnInteractionsOn",
      27U,
      "Interaction class designator",
      takeOrderValue);
  auto const afterTurnOn = readTextFile(publisherReportFile);
  REQUIRE(afterTurnOn.ends_with(turnOnRecord));
  REQUIRE(publisherReports.turnInteractionsOnReports.empty());
  while (publisher->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(publisherReports.turnInteractionsOnReports.size() == 1U);
  REQUIRE(publisherReports.turnInteractionsOnReports.front().interactionClass ==
          takeOrder);

  // §5.17 follows the subscriber's declaration transition and is the next
  // record in the publisher's selected file before its callback is evoked.
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(takeOrder));
  auto const turnOffRecord = advisoryRecord(
      5U,
      "TurnInteractionsOff",
      27U,
      "Interaction class designator",
      takeOrderValue);
  REQUIRE(readTextFile(publisherReportFile) == afterTurnOn + turnOffRecord);
  REQUIRE(publisherReports.turnInteractionsOffReports.empty());
  while (publisher->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(publisherReports.turnInteractionsOffReports.size() == 1U);
  REQUIRE(publisherReports.turnInteractionsOffReports.front().interactionClass ==
          takeOrder);

  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
