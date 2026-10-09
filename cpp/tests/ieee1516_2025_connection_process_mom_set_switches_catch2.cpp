#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "ieee1516_2025_connection_test_support.hpp"
#include "ieee1516_2025_process_fom_test_support.hpp"
#include "process_public_service_fixture.hpp"

namespace {
using rti1516_2025::ConfigurationResult;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIinternalError;
using rti1516_2025::RtiConfiguration;
using rti1516_2025::VariableLengthData;
using TestFederateAmbassador = NullFederateAmbassador;
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportSession;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceOperation;
using umbra::test::connection_support_2025::makeRti;
using umbra::test::connection_support_2025::temporaryServiceReportDirectory;
using umbra::test::process_fom_support_2025::composedProcessDefinition;
}  // namespace
TEST_CASE(
    "RTIambassador carries HLAsetSwitches adjustments through a configured process endpoint",
    "[integration][development-profile][foundation][federation-management][mom][switches]"
    "[mom-process-set-switches][process-mom-set-switches][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-interaction-class-handle]"
    "[rti.service.get-parameter-handle][rti.service.send-interaction]"
    "[rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-object-class-relevance-advisory-switch]"
    "[rti.service.get-attribute-relevance-advisory-switch]"
    "[rti.service.get-attribute-scope-advisory-switch]"
    "[rti.service.get-interaction-relevance-advisory-switch]"
    "[rti.service.get-convey-region-designator-sets-switch]"
    "[rti.service.get-dimension-handle][rti.service.get-dimension-upper-bound]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportServiceDispatcher;
  using umbra::detail::ProcessTransportSession;

  auto const reportDirectory = temporaryServiceReportDirectory();
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

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-mom-set-switches-execution";
  constexpr wchar_t const* federateName =
      L"public-process-mom-set-switches-federate";
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
          {"public-process-mom-set-switches-server", 0x9E01U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      std::wstring stableReportServiceFile;
      auto assertStableReportServiceFile = [&] {
        auto const member = registry.memberByName(federationName, federateName);
        if (!member) {
          throw std::runtime_error(
              "The process HLAsetSwitches test lost its joined member.");
        }
        auto const momObject = registry.joinedFederateMomObjectFor(
            federationName, member->id);
        if (!momObject || momObject->reportServiceFile.empty()) {
          throw std::runtime_error(
              "The process HLAsetSwitches Join did not publish HLAreportServiceFile.");
        }
        std::filesystem::path const path{momObject->reportServiceFile};
        if (!path.is_absolute() || !std::filesystem::exists(path)) {
          throw std::runtime_error(
              "The process HLAreportServiceFile is not an existing absolute path.");
        }
        if (stableReportServiceFile.empty()) {
          stableReportServiceFile = momObject->reportServiceFile;
        } else if (momObject->reportServiceFile != stableReportServiceFile) {
          throw std::runtime_error(
              "HLAsetSwitches changed the joined federate's static HLAreportServiceFile.");
        }
      };
      auto serveExpected = [&](TransportServiceOperation operation) {
        auto const served = umbra::test::servePrimaryProcessRequest(
            session, handler,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The process MOM set-switches server received an unexpected operation.");
              }
              return handler(request);
            });
        if (served &&
            (operation == TransportServiceOperation::join_federation_execution ||
             operation == TransportServiceOperation::send_interaction)) {
          assertStableReportServiceFile();
        }
        return served;
      };
      auto serveSuccessfulSend = [&] {
        return serveExpected(TransportServiceOperation::send_interaction) &&
            serveExpected(
                TransportServiceOperation::report_successful_void_service_invocation);
      };
      if (!serveExpected(TransportServiceOperation::create_federation_execution) ||
          !serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::get_send_service_reports_to_file_switch) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_object_class_relevance_advisory_switch) ||
          !serveExpected(TransportServiceOperation::get_attribute_relevance_advisory_switch) ||
          !serveExpected(TransportServiceOperation::get_attribute_scope_advisory_switch) ||
          !serveExpected(TransportServiceOperation::get_interaction_relevance_advisory_switch) ||
          !serveExpected(TransportServiceOperation::get_convey_region_designator_sets_switch) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_dimension_handle) ||
          !serveSuccessfulSend() ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::get_dimension_upper_bound) ||
          !serveSuccessfulSend() ||
          !serveExpected(TransportServiceOperation::get_send_service_reports_to_file_switch) ||
          !serveExpected(TransportServiceOperation::get_dimension_upper_bound) ||
          !serveSuccessfulSend() ||
          !serveExpected(TransportServiceOperation::get_send_service_reports_to_file_switch) ||
          !serveExpected(TransportServiceOperation::get_dimension_upper_bound) ||
          !serveSuccessfulSend() ||
          !serveExpected(TransportServiceOperation::get_send_service_reports_to_file_switch) ||
          !serveExpected(TransportServiceOperation::get_dimension_upper_bound) ||
          !serveSuccessfulSend() ||
          !serveExpected(TransportServiceOperation::get_send_service_reports_to_file_switch) ||
          !serveSuccessfulSend() ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveSuccessfulSend() ||
          !serveExpected(TransportServiceOperation::get_object_class_relevance_advisory_switch) ||
          !serveExpected(TransportServiceOperation::get_attribute_relevance_advisory_switch) ||
          !serveExpected(TransportServiceOperation::get_attribute_scope_advisory_switch) ||
          !serveExpected(TransportServiceOperation::get_interaction_relevance_advisory_switch) ||
          !serveExpected(TransportServiceOperation::get_convey_region_designator_sets_switch) ||
          !serveSuccessfulSend() ||
          !serveExpected(TransportServiceOperation::get_object_class_relevance_advisory_switch) ||
          !serveExpected(TransportServiceOperation::get_attribute_relevance_advisory_switch) ||
          !serveExpected(TransportServiceOperation::get_attribute_scope_advisory_switch) ||
          !serveExpected(TransportServiceOperation::get_interaction_relevance_advisory_switch) ||
          !serveExpected(TransportServiceOperation::get_convey_region_designator_sets_switch) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The process MOM set-switches server did not receive the complete adjustment sequence.");
      }
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
                               L"public-process-mom-set-switches-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  std::optional<ConfigurationResult> connectionResult;
  bool joined = false;
  try {
    connectionResult = rti->connect(federate, HLA_EVOKED, configuration);
    REQUIRE(connectionResult->addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    static_cast<void>(rti->joinFederationExecution(
        federateName, L"public-process-mom-set-switches-type", federationName));
    joined = true;

    REQUIRE_FALSE(rti->getServiceReportingSwitch());
    REQUIRE_FALSE(rti->getSendServiceReportsToFileSwitch());
    auto const reportFilesAtJoin = reportFiles();
    REQUIRE(reportFilesAtJoin.size() == 1U);
    auto const reportFile = reportFilesAtJoin.front();
    REQUIRE(reportFile.is_absolute());
    auto const initialReportText = readReport(reportFile);
    REQUIRE_FALSE(initialReportText.empty());
    auto const setSwitches = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
    auto const serviceReporting = rti->getParameterHandle(
        setSwitches, L"HLAserviceReporting");
    auto const sendServiceReportsToFile = rti->getParameterHandle(
        setSwitches, L"HLAsendServiceReportsToFile");
    auto const objectClassRelevance = rti->getParameterHandle(
        setSwitches, L"HLAobjectClassRelevanceAdvisory");
    auto const attributeRelevance = rti->getParameterHandle(
        setSwitches, L"HLAattributeRelevanceAdvisory");
    auto const attributeScope = rti->getParameterHandle(
        setSwitches, L"HLAattributeScopeAdvisory");
    auto const interactionRelevance = rti->getParameterHandle(
        setSwitches, L"HLAinteractionRelevanceAdvisory");
    auto const conveyRegionDesignatorSets = rti->getParameterHandle(
        setSwitches, L"HLAconveyRegionDesignatorSets");
    REQUIRE(setSwitches.isValid());
    REQUIRE(serviceReporting.isValid());
    REQUIRE(sendServiceReportsToFile.isValid());
    REQUIRE(objectClassRelevance.isValid());
    REQUIRE(attributeRelevance.isValid());
    REQUIRE(attributeScope.isValid());
    REQUIRE(interactionRelevance.isValid());
    REQUIRE(conveyRegionDesignatorSets.isValid());

    auto const initialObjectClassRelevance =
        rti->getObjectClassRelevanceAdvisorySwitch();
    auto const initialAttributeRelevance =
        rti->getAttributeRelevanceAdvisorySwitch();
    auto const initialAttributeScope = rti->getAttributeScopeAdvisorySwitch();
    auto const initialInteractionRelevance =
        rti->getInteractionRelevanceAdvisorySwitch();
    auto const initialConveyRegionDesignatorSets =
        rti->getConveyRegionDesignatorSetsSwitch();

    REQUIRE_THROWS_AS(
        rti->sendInteraction(
            setSwitches, ParameterHandleValueMap{}, VariableLengthData()),
        RTIinternalError);
    auto const barQuantity = rti->getDimensionHandle(L"BarQuantity");
    REQUIRE(barQuantity.isValid());

    auto encodeSwitch = [](bool const enabled) {
      return rti1516_2025::HLAinteger32BE(enabled ? 1 : 0).encode();
    };
    ParameterHandleValueMap const enabledValues{
        {serviceReporting, encodeSwitch(true)}};
    REQUIRE_NOTHROW(rti->sendInteraction(
        setSwitches, enabledValues, VariableLengthData()));
    REQUIRE(rti->getServiceReportingSwitch());
    REQUIRE(rti->getDimensionUpperBound(barQuantity) == 25UL);
    REQUIRE(readReport(reportFile) == initialReportText);
    REQUIRE(reportFiles() == reportFilesAtJoin);

    ParameterHandleValueMap const fileReportingEnabledValues{
        {sendServiceReportsToFile, encodeSwitch(true)}};
    REQUIRE_NOTHROW(rti->sendInteraction(
        setSwitches, fileReportingEnabledValues, VariableLengthData()));
    REQUIRE(rti->getSendServiceReportsToFileSwitch());
    auto const reportTextBeforeFirstEnabledService = readReport(reportFile);
    REQUIRE(rti->getDimensionUpperBound(barQuantity) == 25UL);
    auto const reportTextAfterEnable = readReport(reportFile);
    REQUIRE(reportTextAfterEnable.size() >
            reportTextBeforeFirstEnabledService.size());
    REQUIRE(reportFiles() == reportFilesAtJoin);

    ParameterHandleValueMap const fileReportingDisabledValues{
        {sendServiceReportsToFile, encodeSwitch(false)}};
    REQUIRE_NOTHROW(rti->sendInteraction(
        setSwitches, fileReportingDisabledValues, VariableLengthData()));
    REQUIRE_FALSE(rti->getSendServiceReportsToFileSwitch());
    auto const reportTextAfterDisable = readReport(reportFile);
    REQUIRE(rti->getDimensionUpperBound(barQuantity) == 25UL);
    REQUIRE(readReport(reportFile) == reportTextAfterDisable);
    REQUIRE(reportFiles() == reportFilesAtJoin);

    REQUIRE_NOTHROW(rti->sendInteraction(
        setSwitches, fileReportingEnabledValues, VariableLengthData()));
    REQUIRE(rti->getSendServiceReportsToFileSwitch());
    auto const reportTextBeforeReenabledService = readReport(reportFile);
    REQUIRE(rti->getDimensionUpperBound(barQuantity) == 25UL);
    auto const reportTextAfterReenable = readReport(reportFile);
    REQUIRE(reportTextAfterReenable.size() >
            reportTextBeforeReenabledService.size());
    REQUIRE(reportFiles() == reportFilesAtJoin);

    REQUIRE_NOTHROW(rti->sendInteraction(
        setSwitches, fileReportingDisabledValues, VariableLengthData()));
    REQUIRE_FALSE(rti->getSendServiceReportsToFileSwitch());

    ParameterHandleValueMap const disabledValues{
        {serviceReporting, encodeSwitch(false)}};
    REQUIRE_NOTHROW(rti->sendInteraction(
        setSwitches, disabledValues, VariableLengthData()));
    REQUIRE_FALSE(rti->getServiceReportingSwitch());

    ParameterHandleValueMap const advisorySwitchValues{
        {objectClassRelevance, encodeSwitch(!initialObjectClassRelevance)},
        {attributeRelevance, encodeSwitch(!initialAttributeRelevance)},
        {attributeScope, encodeSwitch(!initialAttributeScope)},
        {interactionRelevance, encodeSwitch(!initialInteractionRelevance)},
        {conveyRegionDesignatorSets,
         encodeSwitch(!initialConveyRegionDesignatorSets)},
    };
    REQUIRE_NOTHROW(rti->sendInteraction(
        setSwitches, advisorySwitchValues, VariableLengthData()));
    REQUIRE(rti->getObjectClassRelevanceAdvisorySwitch() ==
            !initialObjectClassRelevance);
    REQUIRE(rti->getAttributeRelevanceAdvisorySwitch() ==
            !initialAttributeRelevance);
    REQUIRE(rti->getAttributeScopeAdvisorySwitch() == !initialAttributeScope);
    REQUIRE(rti->getInteractionRelevanceAdvisorySwitch() ==
            !initialInteractionRelevance);
    REQUIRE(rti->getConveyRegionDesignatorSetsSwitch() ==
            !initialConveyRegionDesignatorSets);

    ParameterHandleValueMap const restoredAdvisorySwitchValues{
        {objectClassRelevance, encodeSwitch(initialObjectClassRelevance)},
        {attributeRelevance, encodeSwitch(initialAttributeRelevance)},
        {attributeScope, encodeSwitch(initialAttributeScope)},
        {interactionRelevance, encodeSwitch(initialInteractionRelevance)},
        {conveyRegionDesignatorSets,
         encodeSwitch(initialConveyRegionDesignatorSets)},
    };
    REQUIRE_NOTHROW(rti->sendInteraction(
        setSwitches, restoredAdvisorySwitchValues, VariableLengthData()));
    REQUIRE(rti->getObjectClassRelevanceAdvisorySwitch() ==
            initialObjectClassRelevance);
    REQUIRE(rti->getAttributeRelevanceAdvisorySwitch() ==
            initialAttributeRelevance);
    REQUIRE(rti->getAttributeScopeAdvisorySwitch() == initialAttributeScope);
    REQUIRE(rti->getInteractionRelevanceAdvisorySwitch() ==
            initialInteractionRelevance);
    REQUIRE(rti->getConveyRegionDesignatorSetsSwitch() ==
            initialConveyRegionDesignatorSets);

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
    std::error_code ignored;
    std::filesystem::remove_all(reportDirectory, ignored);
    std::rethrow_exception(clientError);
  }
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(joined);
  REQUIRE(connectionResult.has_value());
}
#endif
