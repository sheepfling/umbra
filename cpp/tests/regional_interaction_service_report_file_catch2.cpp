#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <umbra/embedded_profile_configuration.hpp>

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
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The ordinary regional interaction service-report test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
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
  static std::atomic_uint64_t sequence{0U};
  auto const parent = std::filesystem::temp_directory_path();
  for (std::size_t attempt = 0U; attempt != 1024U; ++attempt) {
    auto const path = parent /
        ("umbra-regional-interaction-report-" +
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
  return L"regional-interaction-report-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
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

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct InteractionReport final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    interactionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
    if (onInteraction) {
      onInteraction();
    }
  }

  std::vector<InteractionReport> interactionReports;
  std::function<void()> onInteraction;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded service reporting records accepted regional Send Interaction With Regions before interaction callback",
    "[integration][development-profile][federation-management][interaction-management]"
    "[ddm][mom][service-report-file][service-reporting]"
    "[ordinary-regional-interaction][ordinary-regional-interaction-service-report]"
    "[rti.service.send-interaction-with-regions][rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(reportDirectory.path());
  configuration.withRtiAddress(L"in-process");
  unsigned char const parameterBytes[] = {0x01, 0x02};
  unsigned char const tagBytes[] = {'r', 'e', 'g'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  VariableLengthData const parameterValue(parameterBytes, sizeof(parameterBytes));
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"regional-interaction-report-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"regional-interaction-report-receiver",
      L"receiver",
      federationName));

  // Keep setup calls out of the accepted service-report record.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const parameter = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::temperature_ok);
  auto const dimension = publisher->getDimensionHandle(
      fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE(dimension.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

  auto const publisherRegion = publisher->createRegion(
      rti1516_2025::DimensionHandleSet{dimension});
  auto const receiverRegion = receiver->createRegion(
      rti1516_2025::DimensionHandleSet{dimension});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      dimension,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      dimension,
      RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  auto const filesAtJoin = serviceReportFiles(reportDirectory.path());
  REQUIRE(filesAtJoin.size() == 1U);
  auto const reportFile = filesAtJoin.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  REQUIRE(readTextFile(reportFile) == initialText);

  ParameterHandleValueMap const parameterValues{{parameter, parameterValue}};
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  REQUIRE(receiverReports.interactionReports.empty());

  auto const interactionValue = asAscii(interactionClass.toString());
  auto const parameterDesignator = asAscii(parameter.toString());
  auto const regionValue = asAscii(publisherRegion.toString());
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SendInteractionWithRegions","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionValue +
      R"("},{"HLAargumentType":40,"HLAargumentName":"Constrained set of interaction parameter designator and value pairs","HLAargumentValue":{")" +
      parameterDesignator +
      R"(":"AQI="}},{"HLAargumentType":43,"HLAargumentName":"Set of region designators","HLAargumentValue":[")" +
      regionValue +
      R"("]},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"cmVn"},{"HLAargumentType":34,"HLAargumentName":"Optional timestamp","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE(serviceReportFiles(reportDirectory.path()) == filesAtJoin);

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  bool reportPresentAtCallbackEntry = false;
  receiverReports.onInteraction = [&] {
    reportPresentAtCallbackEntry =
        readTextFile(reportFile) == initialText + expectedRecord;
  };
  drain(*receiver);
  REQUIRE(reportPresentAtCallbackEntry);
  REQUIRE(receiverReports.interactionReports.size() == 1U);

  auto const& interaction = receiverReports.interactionReports.front();
  REQUIRE(interaction.interactionClass == interactionClass);
  REQUIRE(interaction.parameterValues.size() == 1U);
  REQUIRE(interaction.parameterValues.contains(parameter));
  REQUIRE(variableLengthDataBytes(interaction.parameterValues.at(parameter)) ==
          std::vector<unsigned char>(
              parameterBytes, parameterBytes + sizeof(parameterBytes)));
  REQUIRE(variableLengthDataBytes(interaction.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(interaction.transportationType ==
          receiver->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE(interaction.producingFederate == publisherHandle);
  REQUIRE(interaction.sentRegionsSupplied);
  REQUIRE(interaction.sentRegions == RegionHandleSet{publisherRegion});
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
