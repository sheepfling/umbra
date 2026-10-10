#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64Interval.h>

#include <umbra/embedded_profile_configuration.hpp>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/fom_validation.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "process_public_service_fixture.hpp"

namespace {

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RtiConfiguration;
using rti1516_2025::RTIinternalError;
using rti1516_2025::VariableLengthData;
using rti1516_2025::ConfigurationResult;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::ParameterHandleValueMap;

using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FomCompositionStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::FomValidationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::ProcessTransportSession;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceOperation;

using TestFederateAmbassador = NullFederateAmbassador;

std::filesystem::path processResourcePath(
    std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relative;
}

PrevalidatedFomModule validatedProcessModule(
    std::filesystem::path const& source,
    FomModuleKind kind,
    std::wstring designator) {
  LibXml2FomValidator validator;
  auto result = validator.validate({
      source,
      processResourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      kind,
      std::move(designator),
      L"IEEE1516-DIF-2025.xsd",
  });
  if (result.status != FomValidationStatus::valid || !result.module) {
    throw std::runtime_error("The public process FOM did not validate.");
  }
  return *result.module;
}

FederationDefinition composedProcessDefinition(
    bool const allowRelaxedDdm = false) {
  std::vector<PrevalidatedFomModule> modules{
      validatedProcessModule(
          processResourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:public-process-mim"),
      validatedProcessModule(
          processResourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:public-process-restaurant"),
  };
  if (allowRelaxedDdm) {
    auto const relaxedDdmFom =
        std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
        "data" / "allow-relaxed-ddm-enabled-fom.xml";
    modules.push_back(validatedProcessModule(
        relaxedDdmFom,
        FomModuleKind::fom,
        L"urn:umbra:test:public-process-relaxed-ddm"));
  }
  LibXml2FomModuleComposer composer(
      processResourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid || !result.catalog ||
      !result.fdd) {
    throw std::runtime_error("The public process FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path temporaryServiceReportDirectory() {
  static std::atomic_uint64_t next{0};
  for (;;) {
    auto const candidate = std::filesystem::temp_directory_path() /
        ("umbra-service-report-configuration-" + std::to_string(++next));
    if (std::filesystem::create_directory(candidate)) {
      return candidate;
    }
  }
}

TEST_CASE(
    "RTIambassadors allocate independent process service-report files for simultaneous joined federates",
    "[integration][foundation][federation-management][mom][service-report-file]"
    "[service-reporting][service-report-file-multifederate][transport][process-boundary]"
    "[public-endpoint][2025][rti.service.connect]"
    "[rti.service.create-federation-execution][rti.service.join-federation-execution]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportServiceDispatcher;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  auto const reportDirectory = temporaryServiceReportDirectory();
  constexpr wchar_t const* federationName =
      L"process-service-report-multifederate-execution";
  constexpr wchar_t const* firstFederateName =
      L"process-service-report-multifederate-first";
  constexpr wchar_t const* secondFederateName =
      L"process-service-report-multifederate-second";

  auto reportFiles = [&] {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    if (!std::filesystem::exists(reportDirectory, error)) {
      return files;
    }
    for (auto const& entry : std::filesystem::directory_iterator(reportDirectory, error)) {
      if (!error && entry.is_regular_file(error)) {
        files.push_back(entry.path());
      }
    }
    std::sort(files.begin(), files.end());
    return files;
  };
  auto readReport = [](std::filesystem::path const& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
  };

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
      auto serve = [&](ProcessTransportSession& session,
                       auto const& handler,
                       TransportServiceOperation operation,
                       char const* description) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };

      auto firstConnection = listener->accept(
          nullptr,
          {"process-service-report-multifederate-server", 0x9E21U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession firstSession(firstConnection);
      auto firstHandler = service.handlerFor(firstSession);
      serve(firstSession, firstHandler,
            TransportServiceOperation::create_federation_execution,
            "The process service-report multifederate server lost Create.");
      serve(firstSession, firstHandler,
            TransportServiceOperation::join_federation_execution,
            "The process service-report multifederate server lost first Join.");

      auto secondConnection = listener->accept(
          nullptr,
          {"process-service-report-multifederate-server", 0x9E22U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession secondSession(secondConnection);
      auto secondHandler = service.handlerFor(secondSession);
      serve(secondSession, secondHandler,
            TransportServiceOperation::join_federation_execution,
            "The process service-report multifederate server lost second Join.");
      serve(firstSession, firstHandler,
            TransportServiceOperation::resign_federation_execution,
            "The process service-report multifederate server lost first Resign.");
      serve(secondSession, secondHandler,
            TransportServiceOperation::resign_federation_execution,
            "The process service-report multifederate server lost second Resign.");
      service.detach(firstSession);
      service.detach(secondSession);
      firstConnection->close();
      secondConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador firstFederate;
  TestFederateAmbassador secondFederate;
  auto firstRti = makeRti();
  auto secondRti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-service-report-multifederate-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool firstJoined = false;
  bool secondJoined = false;
  try {
    REQUIRE(firstRti->connect(firstFederate, HLA_EVOKED, configuration).addressUsed);
    firstRti->createFederationExecution(federationName, L"server-owned-fom.xml");
    REQUIRE(firstRti->joinFederationExecution(
                firstFederateName,
                L"process-service-report-multifederate-first-type",
                federationName)
                .isValid());
    firstJoined = true;

    REQUIRE(secondRti->connect(secondFederate, HLA_EVOKED, configuration).addressUsed);
    REQUIRE(secondRti->joinFederationExecution(
                secondFederateName,
                L"process-service-report-multifederate-second-type",
                federationName)
                .isValid());
    secondJoined = true;

    auto const files = reportFiles();
    REQUIRE(files.size() == 2U);
    for (auto const& file : files) {
      REQUIRE(file.is_absolute());
      REQUIRE(file.lexically_normal() == file);
      REQUIRE(file.parent_path() ==
              std::filesystem::absolute(reportDirectory).lexically_normal());
      REQUIRE(std::filesystem::exists(file));
    }
    auto const firstFile = std::find_if(
        files.begin(), files.end(), [&](std::filesystem::path const& file) {
          return readReport(file).find("process-service-report-multifederate-first") !=
              std::string::npos;
        });
    auto const secondFile = std::find_if(
        files.begin(), files.end(), [&](std::filesystem::path const& file) {
          return readReport(file).find("process-service-report-multifederate-second") !=
              std::string::npos;
        });
    REQUIRE(firstFile != files.end());
    REQUIRE(secondFile != files.end());
    REQUIRE(firstFile != secondFile);
    REQUIRE(readReport(*firstFile).find("HLAfederateName") != std::string::npos);
    REQUIRE(readReport(*secondFile).find("HLAfederateName") != std::string::npos);

    auto const firstText = readReport(*firstFile);
    auto const secondText = readReport(*secondFile);
    firstRti->resignFederationExecution(NO_ACTION);
    firstJoined = false;
    REQUIRE(reportFiles() == files);
    REQUIRE(readReport(*firstFile) == firstText);
    REQUIRE(readReport(*secondFile) == secondText);
    secondRti->resignFederationExecution(NO_ACTION);
    secondJoined = false;
    firstRti->disconnect();
    secondRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (firstJoined) {
      try {
        firstRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (secondJoined) {
      try {
        secondRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      firstRti->disconnect();
    } catch (...) {
    }
    try {
      secondRti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(firstJoined);
  REQUIRE_FALSE(secondJoined);
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
}

TEST_CASE(
    "RTIambassador preserves process service-report file identity across switch cycles",
    "[integration][foundation][federation-management][mom][service-report-file]"
    "[service-reporting][service-report-file-lifecycle][transport][process-boundary]"
    "[public-endpoint][2025][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-object-class-handle][rti.service.get-dimension-handle]"
    "[rti.service.get-dimension-name][rti.service.get-dimension-upper-bound]"
    "[rti.service.get-order-name][rti.service.get-order-type]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportServiceDispatcher;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  auto const reportDirectory = temporaryServiceReportDirectory();
  constexpr wchar_t const* federationName =
      L"process-service-report-switch-cycle-execution";
  constexpr wchar_t const* federateName =
      L"process-service-report-switch-cycle-federate";

  auto reportFiles = [&] {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    if (!std::filesystem::exists(reportDirectory, error)) {
      return files;
    }
    for (auto const& entry : std::filesystem::directory_iterator(reportDirectory, error)) {
      if (!error && entry.is_regular_file(error)) {
        files.push_back(entry.path());
      }
    }
    std::sort(files.begin(), files.end());
    return files;
  };
  auto readReport = [](std::filesystem::path const& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
  };

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
      auto connection = listener->accept(
          nullptr,
          {"process-service-report-switch-cycle-server", 0x9E01U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serve = [&](TransportServiceOperation operation, char const* message) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(message);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(message);
        }
      };
      serve(TransportServiceOperation::create_federation_execution,
            "The process service-report server lost Create.");
      serve(TransportServiceOperation::join_federation_execution,
            "The process service-report server lost Join.");
      serve(TransportServiceOperation::get_service_reporting_switch,
            "The process service-report server lost the initial service switch query.");
      serve(TransportServiceOperation::get_send_service_reports_to_file_switch,
            "The process service-report server lost the initial file switch query.");
      serve(TransportServiceOperation::set_service_reporting_switch,
            "The process service-report server lost the service switch enable.");
      serve(TransportServiceOperation::get_service_reporting_switch,
            "The process service-report server lost the enabled service switch query.");
      serve(TransportServiceOperation::set_send_service_reports_to_file_switch,
            "The process service-report server lost the file switch enable.");
      serve(TransportServiceOperation::get_send_service_reports_to_file_switch,
            "The process service-report server lost the enabled file switch query.");
      serve(TransportServiceOperation::get_object_class_handle,
            "The process service-report server lost the first report-producing lookup.");
      serve(TransportServiceOperation::get_dimension_handle,
            "The process service-report server lost the dimension-handle report producer.");
      serve(TransportServiceOperation::get_dimension_name,
            "The process service-report server lost the dimension-name report producer.");
      serve(TransportServiceOperation::get_dimension_upper_bound,
            "The process service-report server lost the dimension upper-bound report producer.");
      serve(TransportServiceOperation::report_successful_service_invocation,
            "The process service-report server lost the local order-name report producer.");
      serve(TransportServiceOperation::report_successful_service_invocation,
            "The process service-report server lost the local order-type report producer.");
      serve(TransportServiceOperation::set_send_service_reports_to_file_switch,
            "The process service-report server lost the file switch disable.");
      serve(TransportServiceOperation::get_send_service_reports_to_file_switch,
            "The process service-report server lost the disabled file switch query.");
      serve(TransportServiceOperation::get_object_class_handle,
            "The process service-report server lost the suppressed lookup.");
      serve(TransportServiceOperation::set_send_service_reports_to_file_switch,
            "The process service-report server lost the file switch re-enable.");
      serve(TransportServiceOperation::get_send_service_reports_to_file_switch,
            "The process service-report server lost the re-enabled file switch query.");
      serve(TransportServiceOperation::get_object_class_handle,
            "The process service-report server lost the resumed lookup.");
      serve(TransportServiceOperation::set_service_reporting_switch,
            "The process service-report server lost the service switch disable.");
      serve(TransportServiceOperation::get_service_reporting_switch,
            "The process service-report server lost the disabled service switch query.");
      serve(TransportServiceOperation::get_object_class_handle,
            "The process service-report server lost the service-suppressed lookup.");
      serve(TransportServiceOperation::set_service_reporting_switch,
            "The process service-report server lost the service switch re-enable.");
      serve(TransportServiceOperation::get_service_reporting_switch,
            "The process service-report server lost the resumed service switch query.");
      serve(TransportServiceOperation::get_object_class_handle,
            "The process service-report server lost the final lookup.");
      serve(TransportServiceOperation::resign_federation_execution,
            "The process service-report server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-service-report-switch-cycle-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool joined = false;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    REQUIRE(rti->joinFederationExecution(
                federateName, L"process-service-report-switch-cycle-type", federationName)
                .isValid());
    joined = true;

    REQUIRE_FALSE(rti->getServiceReportingSwitch());
    REQUIRE_FALSE(rti->getSendServiceReportsToFileSwitch());
    auto files = reportFiles();
    REQUIRE(files.size() == 1U);
    auto const reportFile = files.front();
    REQUIRE(reportFile.is_absolute());
    auto const initialText = readReport(reportFile);
    REQUIRE(initialText.find(R"("HLAservice":"GetDimensionHandle")") ==
            std::string::npos);

    REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
    REQUIRE(rti->getServiceReportingSwitch());
    REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
    REQUIRE(rti->getSendServiceReportsToFileSwitch());
    REQUIRE_NOTHROW(rti->getObjectClassHandle(L"HLAobjectRoot"));
    auto const barQuantity = rti->getDimensionHandle(L"BarQuantity");
    REQUIRE(barQuantity.isValid());
    REQUIRE(rti->getDimensionName(barQuantity) == L"BarQuantity");
    REQUIRE(rti->getDimensionUpperBound(barQuantity) == 25UL);
    REQUIRE(rti->getOrderName(rti1516_2025::RECEIVE) == L"Receive");
    REQUIRE(rti->getOrderType(L"TimeStamp") == rti1516_2025::TIMESTAMP);
    std::string dimensionHandleText;
    auto const dimensionHandleWideText = barQuantity.toString();
    dimensionHandleText.reserve(dimensionHandleWideText.size());
    for (wchar_t const character : dimensionHandleWideText) {
      dimensionHandleText.push_back(static_cast<char>(character));
    }
    auto const expectedDimensionHandleRecord =
        std::string{
            R"("HLAserialNumber":1,"HLAreturnedArgument":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")"} +
        dimensionHandleText +
        R"("}],"HLAservice":"GetDimensionHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Dimension name","HLAargumentValue":"BarQuantity"}])";
    auto const expectedDimensionNameRecord =
        std::string{
            R"("HLAserialNumber":2,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Dimension name","HLAargumentValue":"BarQuantity"}],"HLAservice":"GetDimensionName","HLAsuppliedArguments":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")"} +
        dimensionHandleText + R"("}])";
    auto const expectedDimensionUpperBoundRecord =
        std::string{
            R"("HLAserialNumber":3,"HLAreturnedArgument":[{"HLAargumentType":35,"HLAargumentName":"Dimension upper bound","HLAargumentValue":25}],"HLAservice":"GetDimensionUpperBound","HLAsuppliedArguments":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")"} +
        dimensionHandleText + R"("}])";
    auto const expectedOrderNameRecord =
        R"("HLAserialNumber":4,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Order name","HLAargumentValue":"Receive"}],"HLAservice":"GetOrderName","HLAsuppliedArguments":[{"HLAargumentType":38,"HLAargumentName":"Order type","HLAargumentValue":"RECEIVE"}])";
    auto const expectedOrderTypeRecord =
        R"("HLAserialNumber":5,"HLAreturnedArgument":[{"HLAargumentType":38,"HLAargumentName":"Order type","HLAargumentValue":"TIMESTAMP"}],"HLAservice":"GetOrderType","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Order name","HLAargumentValue":"TimeStamp"}])";
    auto const enabledText = readReport(reportFile);
    REQUIRE(enabledText.size() > initialText.size());
    REQUIRE(enabledText.find(expectedDimensionHandleRecord) !=
            std::string::npos);
    REQUIRE(enabledText.find(expectedDimensionNameRecord) !=
            std::string::npos);
    REQUIRE(enabledText.find(expectedDimensionUpperBoundRecord) !=
            std::string::npos);
    REQUIRE(enabledText.find(expectedOrderNameRecord) != std::string::npos);
    REQUIRE(enabledText.find(expectedOrderTypeRecord) != std::string::npos);
    REQUIRE(reportFiles() == std::vector<std::filesystem::path>{reportFile});

    REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
    REQUIRE_FALSE(rti->getSendServiceReportsToFileSwitch());
    REQUIRE_NOTHROW(rti->getObjectClassHandle(L"HLAobjectRoot"));
    REQUIRE(readReport(reportFile) == enabledText);
    REQUIRE(reportFiles() == std::vector<std::filesystem::path>{reportFile});

    REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
    REQUIRE(rti->getSendServiceReportsToFileSwitch());
    REQUIRE_NOTHROW(rti->getObjectClassHandle(L"HLAobjectRoot"));
    auto const fileReenabledText = readReport(reportFile);
    REQUIRE(fileReenabledText.size() > enabledText.size());

    REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
    REQUIRE_FALSE(rti->getServiceReportingSwitch());
    REQUIRE_NOTHROW(rti->getObjectClassHandle(L"HLAobjectRoot"));
    REQUIRE(readReport(reportFile) == fileReenabledText);

    REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
    REQUIRE(rti->getServiceReportingSwitch());
    REQUIRE_NOTHROW(rti->getObjectClassHandle(L"HLAobjectRoot"));
    REQUIRE(readReport(reportFile).size() > fileReenabledText.size());
    REQUIRE(reportFiles() == std::vector<std::filesystem::path>{reportFile});

    rti->resignFederationExecution(NO_ACTION);
    joined = false;
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (joined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(joined);
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
}

TEST_CASE(
    "RTIambassador reports available class dimensions through the process service-report file",
    "[integration][foundation][federation-management][mom][service-report-file]"
    "[service-reporting][support-services][ddm][transport][process-boundary]"
    "[public-endpoint][2025][process-service-report-available-dimensions]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-object-class-handle][rti.service.get-interaction-class-handle]"
    "[rti.service.get-dimension-handle][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  auto const reportDirectory = temporaryServiceReportDirectory();
  constexpr wchar_t const* federationName =
      L"process-available-dimensions-report-execution";
  constexpr wchar_t const* federateName =
      L"process-available-dimensions-report-federate";

  auto reportFiles = [&] {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    if (!std::filesystem::exists(reportDirectory, error)) {
      return files;
    }
    for (auto const& entry : std::filesystem::directory_iterator(reportDirectory, error)) {
      if (!error && entry.is_regular_file(error)) {
        files.push_back(entry.path());
      }
    }
    std::sort(files.begin(), files.end());
    return files;
  };
  auto readReport = [](std::filesystem::path const& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
  };

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
      auto connection = listener->accept(
          nullptr,
          {"process-available-dimensions-report-server", 0x9E02U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serve = [&](TransportServiceOperation operation, char const* message) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(message);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(message);
        }
      };
      serve(TransportServiceOperation::create_federation_execution,
            "The available-dimensions process server lost Create.");
      serve(TransportServiceOperation::join_federation_execution,
            "The available-dimensions process server lost Join.");
      serve(TransportServiceOperation::get_service_reporting_switch,
            "The available-dimensions process server lost the initial service-switch query.");
      serve(TransportServiceOperation::get_send_service_reports_to_file_switch,
            "The available-dimensions process server lost the initial file-switch query.");
      serve(TransportServiceOperation::get_object_class_handle,
            "The available-dimensions process server lost the object-class lookup.");
      serve(TransportServiceOperation::get_interaction_class_handle,
            "The available-dimensions process server lost the interaction-class lookup.");
      serve(TransportServiceOperation::get_dimension_handle,
            "The available-dimensions process server lost the BarQuantity lookup.");
      serve(TransportServiceOperation::get_dimension_handle,
            "The available-dimensions process server lost the ServerId lookup.");
      serve(TransportServiceOperation::set_service_reporting_switch,
            "The available-dimensions process server lost the service-switch enable.");
      serve(TransportServiceOperation::get_service_reporting_switch,
            "The available-dimensions process server lost the enabled service-switch query.");
      serve(TransportServiceOperation::set_send_service_reports_to_file_switch,
            "The available-dimensions process server lost the file-switch enable.");
      serve(TransportServiceOperation::get_send_service_reports_to_file_switch,
            "The available-dimensions process server lost the enabled file-switch query.");
      serve(TransportServiceOperation::get_available_dimensions_for_object_class,
            "The available-dimensions process server lost the object-class report producer.");
      serve(TransportServiceOperation::get_available_dimensions_for_interaction_class,
            "The available-dimensions process server lost the interaction-class report producer.");
      serve(TransportServiceOperation::resign_federation_execution,
            "The available-dimensions process server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-available-dimensions-report-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool joined = false;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    REQUIRE(rti->joinFederationExecution(
                federateName, L"process-available-dimensions-report-type", federationName)
                .isValid());
    joined = true;
    REQUIRE_FALSE(rti->getServiceReportingSwitch());
    REQUIRE_FALSE(rti->getSendServiceReportsToFileSwitch());

    auto const drink =
        rti->getObjectClassHandle(L"HLAobjectRoot.Food.Drink");
    auto const mainCourseServed = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
    auto const barQuantity = rti->getDimensionHandle(L"BarQuantity");
    auto const serverId = rti->getDimensionHandle(L"ServerId");
    REQUIRE(drink.isValid());
    REQUIRE(mainCourseServed.isValid());
    REQUIRE(barQuantity.isValid());
    REQUIRE(serverId.isValid());

    auto files = reportFiles();
    REQUIRE(files.size() == 1U);
    auto const reportFile = files.front();
    REQUIRE(reportFile.is_absolute());
    auto const initialText = readReport(reportFile);

    REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
    REQUIRE(rti->getServiceReportingSwitch());
    REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
    REQUIRE(rti->getSendServiceReportsToFileSwitch());
    REQUIRE(rti->getAvailableDimensionsForObjectClass(drink) ==
            rti1516_2025::DimensionHandleSet{barQuantity});
    REQUIRE(rti->getAvailableDimensionsForInteractionClass(mainCourseServed) ==
            rti1516_2025::DimensionHandleSet{serverId});

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
    auto const drinkValue = asAscii(drink.toString());
    auto const mainCourseServedValue = asAscii(mainCourseServed.toString());
    auto const barQuantityValue = asAscii(barQuantity.toString());
    auto const serverIdValue = asAscii(serverId.toString());
    auto const expectedObjectDimensionsRecord =
        std::string{
            R"("HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":11,"HLAargumentName":"A set of dimension handles","HLAargumentValue":[")" +
        barQuantityValue +
        R"("]}],"HLAservice":"GetAvailableDimensionsForObjectClass","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class handle","HLAargumentValue":")" +
        drinkValue + R"("}])"};
    auto const expectedInteractionDimensionsRecord =
        std::string{
            R"("HLAserialNumber":1,"HLAreturnedArgument":[{"HLAargumentType":11,"HLAargumentName":"A set of dimension handles","HLAargumentValue":[")" +
        serverIdValue +
        R"("]}],"HLAservice":"GetAvailableDimensionsForInteractionClass","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class handle","HLAargumentValue":")" +
        mainCourseServedValue + R"("}])"};
    auto const reportText = readReport(reportFile);
    REQUIRE(reportText.size() > initialText.size());
    REQUIRE(reportText.find(expectedObjectDimensionsRecord) != std::string::npos);
    REQUIRE(reportText.find(expectedInteractionDimensionsRecord) != std::string::npos);
    REQUIRE(reportFiles() == std::vector<std::filesystem::path>{reportFile});

    rti->resignFederationExecution(NO_ACTION);
    joined = false;
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (joined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(joined);
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
}

TEST_CASE(
    "RTIambassador reports region dimension sets through the process service-report file",
    "[integration][foundation][federation-management][mom][service-report-file]"
    "[service-reporting][support-services][ddm][transport][process-boundary]"
    "[public-endpoint][2025][process-service-report-region-dimension-set]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.create-region]"
    "[rti.service.get-dimension-handle-set][rti.service.get-dimension-handle]"
    "[rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  auto const reportDirectory = temporaryServiceReportDirectory();
  constexpr wchar_t const* federationName =
      L"process-region-dimension-set-report-execution";
  constexpr wchar_t const* federateName =
      L"process-region-dimension-set-report-federate";

  auto reportFiles = [&] {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    if (!std::filesystem::exists(reportDirectory, error)) {
      return files;
    }
    for (auto const& entry : std::filesystem::directory_iterator(reportDirectory, error)) {
      if (!error && entry.is_regular_file(error)) {
        files.push_back(entry.path());
      }
    }
    std::sort(files.begin(), files.end());
    return files;
  };
  auto readReport = [](std::filesystem::path const& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
  };

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
      auto connection = listener->accept(
          nullptr,
          {"process-region-dimension-set-report-server", 0x9E03U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serve = [&](TransportServiceOperation operation, char const* message) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(message);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(message);
        }
      };
      serve(TransportServiceOperation::create_federation_execution,
            "The region-dimension-set process server lost Create.");
      serve(TransportServiceOperation::join_federation_execution,
            "The region-dimension-set process server lost Join.");
      serve(TransportServiceOperation::get_service_reporting_switch,
            "The region-dimension-set process server lost the initial service-switch query.");
      serve(TransportServiceOperation::get_send_service_reports_to_file_switch,
            "The region-dimension-set process server lost the initial file-switch query.");
      serve(TransportServiceOperation::get_dimension_handle,
            "The region-dimension-set process server lost the BarQuantity lookup.");
      serve(TransportServiceOperation::create_region,
            "The region-dimension-set process server lost region creation.");
      serve(TransportServiceOperation::set_service_reporting_switch,
            "The region-dimension-set process server lost the service-switch enable.");
      serve(TransportServiceOperation::get_service_reporting_switch,
            "The region-dimension-set process server lost the enabled service-switch query.");
      serve(TransportServiceOperation::set_send_service_reports_to_file_switch,
            "The region-dimension-set process server lost the file-switch enable.");
      serve(TransportServiceOperation::get_send_service_reports_to_file_switch,
            "The region-dimension-set process server lost the enabled file-switch query.");
      serve(TransportServiceOperation::get_dimension_handle_set,
            "The region-dimension-set process server lost GetDimensionHandleSet.");
      serve(TransportServiceOperation::resign_federation_execution,
            "The region-dimension-set process server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-region-dimension-set-report-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool joined = false;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    REQUIRE(rti->joinFederationExecution(
                federateName, L"process-region-dimension-set-type", federationName)
                .isValid());
    joined = true;
    REQUIRE_FALSE(rti->getServiceReportingSwitch());
    REQUIRE_FALSE(rti->getSendServiceReportsToFileSwitch());

    auto const barQuantity = rti->getDimensionHandle(L"BarQuantity");
    REQUIRE(barQuantity.isValid());
    auto const region = rti->createRegion(
        rti1516_2025::DimensionHandleSet{barQuantity});
    REQUIRE(region.isValid());

    auto files = reportFiles();
    REQUIRE(files.size() == 1U);
    auto const reportFile = files.front();
    REQUIRE(reportFile.is_absolute());
    auto const initialText = readReport(reportFile);
    REQUIRE(readReport(reportFile) == initialText);

    REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
    REQUIRE(rti->getServiceReportingSwitch());
    REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
    REQUIRE(rti->getSendServiceReportsToFileSwitch());
    REQUIRE(rti->getDimensionHandleSet(region) ==
            rti1516_2025::DimensionHandleSet{barQuantity});

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
    auto const regionValue = asAscii(region.toString());
    auto const barQuantityValue = asAscii(barQuantity.toString());
    auto const expectedRecord =
        std::string{
            R"({"HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":11,"HLAargumentName":"A set of dimensions","HLAargumentValue":[")" +
        barQuantityValue +
        R"("]}],"HLAservice":"GetDimensionHandleSet","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
        regionValue +
        R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
    REQUIRE(readReport(reportFile) == initialText + expectedRecord);
    REQUIRE(reportFiles() == std::vector<std::filesystem::path>{reportFile});

    rti->resignFederationExecution(NO_ACTION);
    joined = false;
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (joined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(joined);
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
}

TEST_CASE(
    "RTIambassador reports created regions through the process service-report file",
    "[integration][foundation][federation-management][mom][service-report-file]"
    "[service-reporting][data-distribution-management][ddm][transport][process-boundary]"
    "[public-endpoint][2025][process-service-report-region-create]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-dimension-handle]"
    "[rti.service.create-region][rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  auto const reportDirectory = temporaryServiceReportDirectory();
  constexpr wchar_t const* federationName =
      L"process-region-create-report-execution";
  constexpr wchar_t const* federateName =
      L"process-region-create-report-federate";

  auto reportFiles = [&] {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    if (!std::filesystem::exists(reportDirectory, error)) {
      return files;
    }
    for (auto const& entry : std::filesystem::directory_iterator(reportDirectory, error)) {
      if (!error && entry.is_regular_file(error)) {
        files.push_back(entry.path());
      }
    }
    std::sort(files.begin(), files.end());
    return files;
  };
  auto readReport = [](std::filesystem::path const& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
  };

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
      auto connection = listener->accept(
          nullptr,
          {"process-region-create-report-server", 0x9E04U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serve = [&](TransportServiceOperation operation, char const* message) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(message);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(message);
        }
      };
      serve(TransportServiceOperation::create_federation_execution,
            "The region-create process server lost Create.");
      serve(TransportServiceOperation::join_federation_execution,
            "The region-create process server lost Join.");
      serve(TransportServiceOperation::get_dimension_handle,
            "The region-create process server lost the BarQuantity lookup.");
      serve(TransportServiceOperation::set_service_reporting_switch,
            "The region-create process server lost the service-switch enable.");
      serve(TransportServiceOperation::set_send_service_reports_to_file_switch,
            "The region-create process server lost the file-switch enable.");
      serve(TransportServiceOperation::create_region,
            "The region-create process server lost CreateRegion.");
      serve(TransportServiceOperation::resign_federation_execution,
            "The region-create process server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-region-create-report-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool joined = false;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    REQUIRE(rti->joinFederationExecution(
                federateName, L"process-region-create-type", federationName)
                .isValid());
    joined = true;

    auto files = reportFiles();
    REQUIRE(files.size() == 1U);
    auto const reportFile = files.front();
    REQUIRE(reportFile.is_absolute());
    auto const initialText = readReport(reportFile);
    REQUIRE_FALSE(initialText.empty());

    auto const barQuantity = rti->getDimensionHandle(L"BarQuantity");
    REQUIRE(barQuantity.isValid());
    REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
    REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
    auto const region = rti->createRegion(
        rti1516_2025::DimensionHandleSet{barQuantity});
    REQUIRE(region.isValid());

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
    auto const regionValue = asAscii(region.toString());
    auto const barQuantityValue = asAscii(barQuantity.toString());
    auto const expectedRecord =
        std::string{
            R"({"HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":42,"HLAargumentName":"Region designator","HLAargumentValue":")" +
        regionValue +
        R"("}],"HLAservice":"CreateRegion","HLAsuppliedArguments":[{"HLAargumentType":11,"HLAargumentName":"Set of dimension designators","HLAargumentValue":[")" +
        barQuantityValue +
        R"("]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
    REQUIRE(readReport(reportFile) == initialText + expectedRecord);
    REQUIRE(reportFiles() == std::vector<std::filesystem::path>{reportFile});

    rti->resignFederationExecution(NO_ACTION);
    joined = false;
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (joined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(joined);
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
}

TEST_CASE(
    "RTIambassador reports deleted regions through the process service-report file",
    "[integration][foundation][federation-management][mom][service-report-file]"
    "[service-reporting][data-distribution-management][ddm][transport][process-boundary]"
    "[public-endpoint][2025][process-service-report-region-delete]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-dimension-handle]"
    "[rti.service.create-region][rti.service.delete-region]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  auto const reportDirectory = temporaryServiceReportDirectory();
  constexpr wchar_t const* federationName =
      L"process-region-delete-report-execution";
  constexpr wchar_t const* federateName =
      L"process-region-delete-report-federate";

  auto reportFiles = [&] {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    if (!std::filesystem::exists(reportDirectory, error)) {
      return files;
    }
    for (auto const& entry : std::filesystem::directory_iterator(reportDirectory, error)) {
      if (!error && entry.is_regular_file(error)) {
        files.push_back(entry.path());
      }
    }
    std::sort(files.begin(), files.end());
    return files;
  };
  auto readReport = [](std::filesystem::path const& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
  };

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
      auto connection = listener->accept(
          nullptr,
          {"process-region-delete-report-server", 0x9E05U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serve = [&](TransportServiceOperation operation, char const* message) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(message);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(message);
        }
      };
      serve(TransportServiceOperation::create_federation_execution,
            "The region-delete process server lost Create.");
      serve(TransportServiceOperation::join_federation_execution,
            "The region-delete process server lost Join.");
      serve(TransportServiceOperation::get_dimension_handle,
            "The region-delete process server lost the BarQuantity lookup.");
      serve(TransportServiceOperation::create_region,
            "The region-delete process server lost CreateRegion.");
      serve(TransportServiceOperation::set_service_reporting_switch,
            "The region-delete process server lost the service-switch enable.");
      serve(TransportServiceOperation::set_send_service_reports_to_file_switch,
            "The region-delete process server lost the file-switch enable.");
      serve(TransportServiceOperation::delete_region,
            "The region-delete process server lost DeleteRegion.");
      serve(TransportServiceOperation::resign_federation_execution,
            "The region-delete process server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-region-delete-report-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool joined = false;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    REQUIRE(rti->joinFederationExecution(
                federateName, L"process-region-delete-type", federationName)
                .isValid());
    joined = true;

    auto files = reportFiles();
    REQUIRE(files.size() == 1U);
    auto const reportFile = files.front();
    REQUIRE(reportFile.is_absolute());
    auto const initialText = readReport(reportFile);
    REQUIRE_FALSE(initialText.empty());

    auto const barQuantity = rti->getDimensionHandle(L"BarQuantity");
    REQUIRE(barQuantity.isValid());
    auto const region = rti->createRegion(
        rti1516_2025::DimensionHandleSet{barQuantity});
    REQUIRE(region.isValid());
    REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
    REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
    REQUIRE_NOTHROW(rti->deleteRegion(region));

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
    auto const regionValue = asAscii(region.toString());
    auto const expectedRecord =
        std::string{
            R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"DeleteRegion","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region designator","HLAargumentValue":")" +
        regionValue +
        R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
    REQUIRE(readReport(reportFile) == initialText + expectedRecord);
    REQUIRE(reportFiles() == std::vector<std::filesystem::path>{reportFile});

    rti->resignFederationExecution(NO_ACTION);
    joined = false;
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (joined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(joined);
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
}

TEST_CASE(
    "RTIambassador reports committed region modifications through the process service-report file",
    "[integration][foundation][federation-management][mom][service-report-file]"
    "[service-reporting][data-distribution-management][ddm][transport][process-boundary]"
    "[public-endpoint][2025][process-service-report-region-commit]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-dimension-handle]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  auto const reportDirectory = temporaryServiceReportDirectory();
  constexpr wchar_t const* federationName =
      L"process-region-commit-report-execution";
  constexpr wchar_t const* federateName =
      L"process-region-commit-report-federate";

  auto reportFiles = [&] {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    if (!std::filesystem::exists(reportDirectory, error)) {
      return files;
    }
    for (auto const& entry : std::filesystem::directory_iterator(reportDirectory, error)) {
      if (!error && entry.is_regular_file(error)) {
        files.push_back(entry.path());
      }
    }
    std::sort(files.begin(), files.end());
    return files;
  };
  auto readReport = [](std::filesystem::path const& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
  };

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
      auto connection = listener->accept(
          nullptr,
          {"process-region-commit-report-server", 0x9E06U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serve = [&](TransportServiceOperation operation, char const* message) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(message);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(message);
        }
      };
      serve(TransportServiceOperation::create_federation_execution,
            "The region-commit process server lost Create.");
      serve(TransportServiceOperation::join_federation_execution,
            "The region-commit process server lost Join.");
      serve(TransportServiceOperation::get_dimension_handle,
            "The region-commit process server lost the BarQuantity lookup.");
      serve(TransportServiceOperation::create_region,
            "The region-commit process server lost CreateRegion.");
      serve(TransportServiceOperation::set_range_bounds,
            "The region-commit process server lost SetRangeBounds.");
      serve(TransportServiceOperation::set_service_reporting_switch,
            "The region-commit process server lost the service-switch enable.");
      serve(TransportServiceOperation::set_send_service_reports_to_file_switch,
            "The region-commit process server lost the file-switch enable.");
      serve(TransportServiceOperation::commit_region_modifications,
            "The region-commit process server lost CommitRegionModifications.");
      serve(TransportServiceOperation::resign_federation_execution,
            "The region-commit process server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-region-commit-report-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool joined = false;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    REQUIRE(rti->joinFederationExecution(
                federateName, L"process-region-commit-type", federationName)
                .isValid());
    joined = true;

    auto files = reportFiles();
    REQUIRE(files.size() == 1U);
    auto const reportFile = files.front();
    REQUIRE(reportFile.is_absolute());
    auto const initialText = readReport(reportFile);
    REQUIRE_FALSE(initialText.empty());

    auto const barQuantity = rti->getDimensionHandle(L"BarQuantity");
    REQUIRE(barQuantity.isValid());
    auto const region = rti->createRegion(
        rti1516_2025::DimensionHandleSet{barQuantity});
    REQUIRE(region.isValid());
    REQUIRE_NOTHROW(rti->setRangeBounds(
        region, barQuantity, rti1516_2025::RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
    REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
    REQUIRE_NOTHROW(rti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{region}));

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
    auto const regionValue = asAscii(region.toString());
    auto const expectedRecord =
        std::string{
            R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"CommitRegionModifications","HLAsuppliedArguments":[{"HLAargumentType":43,"HLAargumentName":"Set of region designators","HLAargumentValue":[")" +
        regionValue +
        R"("]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
    REQUIRE(readReport(reportFile) == initialText + expectedRecord);
    REQUIRE(reportFiles() == std::vector<std::filesystem::path>{reportFile});

    rti->resignFederationExecution(NO_ACTION);
    joined = false;
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (joined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(joined);
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
}

TEST_CASE(
    "RTIambassador reports range bounds through the process service-report file",
    "[integration][foundation][federation-management][mom][service-report-file]"
    "[service-reporting][data-distribution-management][ddm][transport][process-boundary]"
    "[public-endpoint][2025][process-service-report-region-get-bounds]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-dimension-handle]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications][rti.service.get-range-bounds]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  auto const reportDirectory = temporaryServiceReportDirectory();
  constexpr wchar_t const* federationName =
      L"process-region-get-bounds-report-execution";
  constexpr wchar_t const* federateName =
      L"process-region-get-bounds-report-federate";

  auto reportFiles = [&] {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    if (!std::filesystem::exists(reportDirectory, error)) {
      return files;
    }
    for (auto const& entry : std::filesystem::directory_iterator(reportDirectory, error)) {
      if (!error && entry.is_regular_file(error)) {
        files.push_back(entry.path());
      }
    }
    std::sort(files.begin(), files.end());
    return files;
  };
  auto readReport = [](std::filesystem::path const& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
  };

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
      auto connection = listener->accept(
          nullptr,
          {"process-region-get-bounds-report-server", 0x9E07U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serve = [&](TransportServiceOperation operation, char const* message) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(message);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(message);
        }
      };
      serve(TransportServiceOperation::create_federation_execution,
            "The range-bounds process server lost Create.");
      serve(TransportServiceOperation::join_federation_execution,
            "The range-bounds process server lost Join.");
      serve(TransportServiceOperation::get_dimension_handle,
            "The range-bounds process server lost the BarQuantity lookup.");
      serve(TransportServiceOperation::create_region,
            "The range-bounds process server lost CreateRegion.");
      serve(TransportServiceOperation::set_range_bounds,
            "The range-bounds process server lost SetRangeBounds.");
      serve(TransportServiceOperation::commit_region_modifications,
            "The range-bounds process server lost CommitRegionModifications.");
      serve(TransportServiceOperation::set_service_reporting_switch,
            "The range-bounds process server lost the service-switch enable.");
      serve(TransportServiceOperation::set_send_service_reports_to_file_switch,
            "The range-bounds process server lost the file-switch enable.");
      serve(TransportServiceOperation::get_range_bounds,
            "The range-bounds process server lost GetRangeBounds.");
      serve(TransportServiceOperation::resign_federation_execution,
            "The range-bounds process server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-region-get-bounds-report-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool joined = false;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    REQUIRE(rti->joinFederationExecution(
                federateName, L"process-region-get-bounds-type", federationName)
                .isValid());
    joined = true;

    auto files = reportFiles();
    REQUIRE(files.size() == 1U);
    auto const reportFile = files.front();
    REQUIRE(reportFile.is_absolute());
    auto const initialText = readReport(reportFile);
    REQUIRE_FALSE(initialText.empty());

    auto const barQuantity = rti->getDimensionHandle(L"BarQuantity");
    REQUIRE(barQuantity.isValid());
    auto const region = rti->createRegion(
        rti1516_2025::DimensionHandleSet{barQuantity});
    REQUIRE(region.isValid());
    REQUIRE_NOTHROW(rti->setRangeBounds(
        region, barQuantity, rti1516_2025::RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(rti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{region}));
    REQUIRE(readReport(reportFile) == initialText);

    REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
    REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
    auto const bounds = rti->getRangeBounds(region, barQuantity);
    REQUIRE(bounds.getLowerBound() == 0UL);
    REQUIRE(bounds.getUpperBound() == 1UL);

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
    auto const regionValue = asAscii(region.toString());
    auto const barQuantityValue = asAscii(barQuantity.toString());
    auto const expectedRecord = std::string{
        R"({"HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":41,"HLAargumentName":"Range bounds","HLAargumentValue":{"lower":0,"upper":1}}],"HLAservice":"GetRangeBounds","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
        regionValue +
        R"("},{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
        barQuantityValue +
        R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
    REQUIRE(readReport(reportFile) == initialText + expectedRecord);
    REQUIRE(reportFiles() == std::vector<std::filesystem::path>{reportFile});

    rti->resignFederationExecution(NO_ACTION);
    joined = false;
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (joined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(joined);
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
}

TEST_CASE(
    "RTIambassador reports range-bound changes through the process service-report file",
    "[integration][foundation][federation-management][mom][service-report-file]"
    "[service-reporting][data-distribution-management][ddm][transport][process-boundary]"
    "[public-endpoint][2025][process-service-report-region-set-bounds]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-dimension-handle]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  auto const reportDirectory = temporaryServiceReportDirectory();
  constexpr wchar_t const* federationName =
      L"process-region-set-bounds-report-execution";
  constexpr wchar_t const* federateName =
      L"process-region-set-bounds-report-federate";

  auto reportFiles = [&] {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    if (!std::filesystem::exists(reportDirectory, error)) {
      return files;
    }
    for (auto const& entry : std::filesystem::directory_iterator(reportDirectory, error)) {
      if (!error && entry.is_regular_file(error)) {
        files.push_back(entry.path());
      }
    }
    std::sort(files.begin(), files.end());
    return files;
  };
  auto readReport = [](std::filesystem::path const& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
  };

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
      auto connection = listener->accept(
          nullptr,
          {"process-region-set-bounds-report-server", 0x9E08U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serve = [&](TransportServiceOperation operation, char const* message) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(message);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(message);
        }
      };
      serve(TransportServiceOperation::create_federation_execution,
            "The range-set process server lost Create.");
      serve(TransportServiceOperation::join_federation_execution,
            "The range-set process server lost Join.");
      serve(TransportServiceOperation::get_dimension_handle,
            "The range-set process server lost the BarQuantity lookup.");
      serve(TransportServiceOperation::create_region,
            "The range-set process server lost CreateRegion.");
      serve(TransportServiceOperation::set_range_bounds,
            "The range-set process server lost initial SetRangeBounds.");
      serve(TransportServiceOperation::commit_region_modifications,
            "The range-set process server lost CommitRegionModifications.");
      serve(TransportServiceOperation::set_service_reporting_switch,
            "The range-set process server lost the service-switch enable.");
      serve(TransportServiceOperation::set_send_service_reports_to_file_switch,
            "The range-set process server lost the file-switch enable.");
      serve(TransportServiceOperation::set_range_bounds,
            "The range-set process server lost reported SetRangeBounds.");
      serve(TransportServiceOperation::resign_federation_execution,
            "The range-set process server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-region-set-bounds-report-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool joined = false;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    REQUIRE(rti->joinFederationExecution(
                federateName, L"process-region-set-bounds-type", federationName)
                .isValid());
    joined = true;

    auto files = reportFiles();
    REQUIRE(files.size() == 1U);
    auto const reportFile = files.front();
    REQUIRE(reportFile.is_absolute());
    auto const initialText = readReport(reportFile);
    REQUIRE_FALSE(initialText.empty());

    auto const barQuantity = rti->getDimensionHandle(L"BarQuantity");
    REQUIRE(barQuantity.isValid());
    auto const region = rti->createRegion(
        rti1516_2025::DimensionHandleSet{barQuantity});
    REQUIRE(region.isValid());
    REQUIRE_NOTHROW(rti->setRangeBounds(
        region, barQuantity, rti1516_2025::RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(rti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{region}));
    REQUIRE(readReport(reportFile) == initialText);

    REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
    REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
    REQUIRE_NOTHROW(rti->setRangeBounds(
        region, barQuantity, rti1516_2025::RangeBounds(2UL, 3UL)));

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
    auto const regionValue = asAscii(region.toString());
    auto const barQuantityValue = asAscii(barQuantity.toString());
    auto const expectedRecord = std::string{
        R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SetRangeBounds","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
        regionValue +
        R"("},{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
        barQuantityValue +
        R"("},{"HLAargumentType":35,"HLAargumentName":"Range lower bound","HLAargumentValue":2},{"HLAargumentType":35,"HLAargumentName":"Range upper bound","HLAargumentValue":3}],"HLAsuccessIndicator":true,"HLAexception":null})"};
    REQUIRE(readReport(reportFile) == initialText + expectedRecord);
    REQUIRE(reportFiles() == std::vector<std::filesystem::path>{reportFile});

    rti->resignFederationExecution(NO_ACTION);
    joined = false;
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (joined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(joined);
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
}

TEST_CASE(
    "RTIambassador assigns a new process service-report file after resign and rejoin",
    "[integration][foundation][federation-management][mom][service-report-file]"
    "[service-reporting][service-report-file-lifecycle][process-service-report-file-rejoin]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.resign-federation-execution]"
    "[rti.service.disconnect]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportServiceDispatcher;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  auto const reportDirectory = temporaryServiceReportDirectory();
  constexpr wchar_t const* federationName =
      L"process-service-report-rejoin-execution";
  constexpr wchar_t const* federateName =
      L"process-service-report-rejoin-federate";

  auto reportFiles = [&] {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    if (!std::filesystem::exists(reportDirectory, error)) {
      return files;
    }
    for (auto const& entry :
         std::filesystem::directory_iterator(reportDirectory, error)) {
      if (!error && entry.is_regular_file(error)) {
        files.push_back(entry.path());
      }
    }
    std::sort(files.begin(), files.end());
    return files;
  };
  auto readReport = [](std::filesystem::path const& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
  };

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
      auto connection = listener->accept(
          nullptr,
          {"process-service-report-rejoin-server", 0x9E11U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serve = [&](TransportServiceOperation operation, char const* message) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(message);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(message);
        }
      };
      serve(TransportServiceOperation::create_federation_execution,
            "The process service-report rejoin server lost Create.");
      serve(TransportServiceOperation::join_federation_execution,
            "The process service-report rejoin server lost the first Join.");
      serve(TransportServiceOperation::resign_federation_execution,
            "The process service-report rejoin server lost the first Resign.");
      serve(TransportServiceOperation::join_federation_execution,
            "The process service-report rejoin server lost the second Join.");
      serve(TransportServiceOperation::resign_federation_execution,
            "The process service-report rejoin server lost the second Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-service-report-rejoin-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool joined = false;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");

    REQUIRE(rti->joinFederationExecution(
                federateName, L"process-service-report-rejoin-type", federationName)
                .isValid());
    joined = true;
    auto const firstFiles = reportFiles();
    REQUIRE(firstFiles.size() == 1U);
    auto const firstReport = firstFiles.front();
    REQUIRE(firstReport.is_absolute());
    auto const firstText = readReport(firstReport);
    REQUIRE_FALSE(firstText.empty());
    REQUIRE(firstText.find("HLAfederateName") != std::string::npos);
    REQUIRE(firstText.find("process-service-report-rejoin-federate") !=
            std::string::npos);

    rti->resignFederationExecution(NO_ACTION);
    joined = false;
    REQUIRE(std::filesystem::exists(firstReport));
    REQUIRE(reportFiles() == firstFiles);
    REQUIRE(readReport(firstReport) == firstText);

    REQUIRE(rti->joinFederationExecution(
                federateName, L"process-service-report-rejoin-type", federationName)
                .isValid());
    joined = true;
    auto const secondFiles = reportFiles();
    REQUIRE(secondFiles.size() == 2U);
    auto const second = std::find_if(
        secondFiles.begin(), secondFiles.end(),
        [&](std::filesystem::path const& path) { return path != firstReport; });
    REQUIRE(second != secondFiles.end());
    REQUIRE(second->is_absolute());
    REQUIRE(*second != firstReport);
    auto const secondText = readReport(*second);
    REQUIRE_FALSE(secondText.empty());
    REQUIRE(secondText.find("HLAfederateName") != std::string::npos);
    REQUIRE(secondText.find("process-service-report-rejoin-federate") !=
            std::string::npos);
    REQUIRE(readReport(firstReport) == firstText);

    rti->resignFederationExecution(NO_ACTION);
    joined = false;
    REQUIRE(std::filesystem::exists(firstReport));
    REQUIRE(std::filesystem::exists(*second));
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (joined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(joined);
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
}

TEST_CASE(
    "RTIambassador rejects an unusable process service-report directory before endpoint connection",
    "[integration][foundation][federation-management][mom][service-reporting]"
    "[service-report-file][service-report-file-failure][transport][process-boundary]"
    "[public-endpoint][2025][rti.service.connect]") {
  using umbra::detail::ProcessTransportListener;

  TestFederateAmbassador federate;
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  REQUIRE(listener->address().port != 0U);

  auto const parent = temporaryServiceReportDirectory();
  std::filesystem::create_directories(parent);
  auto const regularFile = parent / "not-a-directory";
  std::ofstream output(regularFile, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output << "not a directory";
  REQUIRE(output.good());
  output.close();

  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"process-service-report-failure")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" +
                               std::to_wstring(listener->address().port))
                           .withAdditionalSettings(
                               L"serviceReportDirectory=" + regularFile.wstring());
  auto rti = makeRti();

  REQUIRE_THROWS_AS(
      rti->connect(federate, HLA_EVOKED, configuration),
      RTIinternalError);
  // The invalid filesystem configuration is rejected before the process
  // endpoint is touched and cannot leave a partially connected ambassador.
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());

  std::error_code ignored;
  std::filesystem::remove_all(parent, ignored);
}

TEST_CASE(
    "RTIambassador reports a deterministic process service-report append failure after file loss",
    "[integration][foundation][federation-management][mom][service-reporting]"
    "[service-report-file][service-report-file-failure][service-report-file-loss]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-object-class-handle]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportServiceDispatcher;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  auto const reportDirectory = temporaryServiceReportDirectory();
  constexpr wchar_t const* federationName =
      L"process-service-report-file-loss-execution";
  constexpr wchar_t const* federateName =
      L"process-service-report-file-loss-federate";

  auto reportFiles = [&] {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    if (!std::filesystem::exists(reportDirectory, error)) {
      return files;
    }
    for (auto const& entry :
         std::filesystem::directory_iterator(reportDirectory, error)) {
      if (!error && entry.is_regular_file(error)) {
        files.push_back(entry.path());
      }
    }
    std::sort(files.begin(), files.end());
    return files;
  };

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
      auto connection = listener->accept(
          nullptr,
          {"process-service-report-file-loss-server", 0x9E12U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serve = [&](TransportServiceOperation operation, char const* message) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(message);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(message);
        }
      };
      serve(TransportServiceOperation::create_federation_execution,
            "The process service-report file-loss server lost Create.");
      serve(TransportServiceOperation::join_federation_execution,
            "The process service-report file-loss server lost Join.");
      serve(TransportServiceOperation::set_service_reporting_switch,
            "The process service-report file-loss server lost the service-report switch enable.");
      serve(TransportServiceOperation::set_send_service_reports_to_file_switch,
            "The process service-report file-loss server lost the file-report switch enable.");
      serve(TransportServiceOperation::get_object_class_handle,
            "The process service-report file-loss server lost the report-producing lookup.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-service-report-file-loss-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool joined = false;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    REQUIRE(rti->joinFederationExecution(
                federateName, L"process-service-report-file-loss-type", federationName)
                .isValid());
    joined = true;
    REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
    REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));

    auto const filesAtJoin = reportFiles();
    REQUIRE(filesAtJoin.size() == 1U);
    auto const reportFile = filesAtJoin.front();
    REQUIRE(reportFile.is_absolute());
    std::error_code removed;
    REQUIRE(std::filesystem::remove(reportFile, removed));
    REQUIRE_FALSE(removed);
    REQUIRE_FALSE(std::filesystem::exists(reportFile));

    REQUIRE_THROWS_AS(
        rti->getObjectClassHandle(L"HLAobjectRoot"),
        RTIinternalError);
    REQUIRE(reportFiles().empty());

    // The server-side append failed before it could produce a success reply;
    // stop issuing services and let local destruction close the failed session.
    rti.reset();
    joined = false;
  } catch (...) {
    clientError = std::current_exception();
    if (joined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  if (serverError) {
    std::rethrow_exception(serverError);
  }
  REQUIRE_FALSE(joined);
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
}


}  // namespace
#endif
