#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <atomic>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/auth/HLAnoCredentials.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>
#include <RTI/encoding/BasicDataElements.h>

#include <umbra/embedded_profile_configuration.hpp>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "process_public_service_fixture.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/fom_validation.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/dimension_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
namespace {

using rti1516_2025::CallbackModel;
using rti1516_2025::ConfigurationResult;
using rti1516_2025::DELETE_OBJECTS;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::HLAnoCredentials;
using rti1516_2025::HLAplainTextPassword;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::NO_ACTION;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RTIinternalError;
using rti1516_2025::RtiConfiguration;
using rti1516_2025::SETTINGS_APPLIED;
using rti1516_2025::SETTINGS_FAILED_TO_PARSE;
using rti1516_2025::SETTINGS_IGNORED;
using rti1516_2025::Unauthorized;
using rti1516_2025::VariableLengthData;

using TestFederateAmbassador = NullFederateAmbassador;

class ProcessTimeFederateAmbassador final : public NullFederateAmbassador {
 public:
  void timeRegulationEnabled(
      rti1516_2025::LogicalTime const& time) override {
    ++timeRegulationEnabledCount;
    timeRegulationEnabledImplementation = time.implementationName();
  }

  void timeConstrainedEnabled(
      rti1516_2025::LogicalTime const& time) override {
    ++timeConstrainedEnabledCount;
    timeConstrainedEnabledImplementation = time.implementationName();
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    ++timeAdvanceGrantCount;
    timeAdvanceGrantImplementation = time.implementationName();
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      timeAdvanceGrantValue = integerTime->getTime();
    }
  }

  void flushQueueGrant(
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::LogicalTime const& optimisticTime) override {
    ++flushQueueGrantCount;
    flushQueueGrantImplementation = time.implementationName();
    flushQueueGrantOptimisticImplementation = optimisticTime.implementationName();
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      flushQueueGrantValue = integerTime->getTime();
    }
    if (auto const* integerOptimisticTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&optimisticTime)) {
      flushQueueGrantOptimisticValue = integerOptimisticTime->getTime();
    }
  }

  std::size_t timeRegulationEnabledCount = 0U;
  std::wstring timeRegulationEnabledImplementation;
  std::size_t timeConstrainedEnabledCount = 0U;
  std::wstring timeConstrainedEnabledImplementation;
  std::size_t timeAdvanceGrantCount = 0U;
  std::wstring timeAdvanceGrantImplementation;
  std::int64_t timeAdvanceGrantValue = -1;
  std::size_t flushQueueGrantCount = 0U;
  std::wstring flushQueueGrantImplementation;
  std::wstring flushQueueGrantOptimisticImplementation;
  std::int64_t flushQueueGrantValue = -1;
  std::int64_t flushQueueGrantOptimisticValue = -1;
};

class ProcessConstrainedEnableWaitFederateAmbassador final
    : public NullFederateAmbassador {
 public:
  void timeAdvanceGrant(
      rti1516_2025::LogicalTime const& time) override {
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      grantedTime.store(integerTime->getTime(), std::memory_order_release);
    }
    grantCount.fetch_add(1U, std::memory_order_acq_rel);
  }

  void timeConstrainedEnabled(
      rti1516_2025::LogicalTime const& time) override {
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      enabledTime.store(integerTime->getTime(), std::memory_order_release);
    }
    callbackCount.fetch_add(1U, std::memory_order_acq_rel);
    changed.notify_all();
  }

  std::atomic_size_t callbackCount{0U};
  std::atomic<std::int64_t> enabledTime{-1};
  std::atomic_size_t grantCount{0U};
  std::atomic<std::int64_t> grantedTime{-1};
  std::mutex mutex;
  std::condition_variable changed;
};

class ProcessSynchronizationFederateAmbassador final
    : public NullFederateAmbassador {
 public:
  void synchronizationPointRegistrationSucceeded(
      std::wstring const& label) override {
    ++registrationSucceededCount;
    registrationLabel = label;
  }

  void synchronizationPointRegistrationFailed(
      std::wstring const& label,
      rti1516_2025::SynchronizationPointFailureReason reason) override {
    ++registrationFailedCount;
    registrationLabel = label;
    registrationFailureReason = reason;
  }

  void announceSynchronizationPoint(
      std::wstring const& label,
      VariableLengthData const& tag) override {
    ++announcementCount;
    announcementLabel = label;
    announcementTag.clear();
    if (tag.size() != 0U) {
      auto const* data = static_cast<std::uint8_t const*>(tag.data());
      announcementTag.assign(data, data + tag.size());
    }
  }

  void federationSynchronized(
      std::wstring const& label,
      rti1516_2025::FederateHandleSet const& failedToSyncSet) override {
    ++synchronizedCount;
    synchronizedLabel = label;
    failedToSyncCount = failedToSyncSet.size();
  }

  std::size_t registrationSucceededCount = 0U;
  std::size_t registrationFailedCount = 0U;
  std::wstring registrationLabel;
  rti1516_2025::SynchronizationPointFailureReason registrationFailureReason =
      rti1516_2025::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE;
  std::size_t announcementCount = 0U;
  std::wstring announcementLabel;
  std::vector<std::uint8_t> announcementTag;
  std::size_t synchronizedCount = 0U;
  std::wstring synchronizedLabel;
  std::size_t failedToSyncCount = 0U;
};

class ProcessSaveFederateAmbassador final : public NullFederateAmbassador {
 public:
  void timeRegulationEnabled(
      rti1516_2025::LogicalTime const&) override {
    ++timeRegulationEnabledCount;
  }

  void initiateFederateSave(std::wstring const& label) override {
    ++initiateCount;
    initiateLabel = label;
  }

  void initiateFederateSave(
      std::wstring const& label,
      rti1516_2025::LogicalTime const& time) override {
    ++timedInitiateCount;
    timedInitiateLabel = label;
    timedInitiateImplementation = time.implementationName();
    timedInitiateValue = time.toString();
  }

  void federationSaved() override { ++savedCount; }

  void federationNotSaved(rti1516_2025::SaveFailureReason reason) override {
    ++notSavedCount;
    notSavedReason = reason;
  }

  void federationSaveStatusResponse(
      rti1516_2025::FederateHandleSaveStatusPairVector const& response) override {
    federationSaveStatusReports.push_back(response);
  }

  std::size_t initiateCount = 0U;
  std::wstring initiateLabel;
  std::size_t timeRegulationEnabledCount = 0U;
  std::size_t timedInitiateCount = 0U;
  std::wstring timedInitiateLabel;
  std::wstring timedInitiateImplementation;
  std::wstring timedInitiateValue;
  std::size_t savedCount = 0U;
  std::size_t notSavedCount = 0U;
  rti1516_2025::SaveFailureReason notSavedReason = rti1516_2025::SAVE_ABORTED;
  std::vector<rti1516_2025::FederateHandleSaveStatusPairVector>
      federationSaveStatusReports;
};

class ProcessRestoreFederateAmbassador final : public NullFederateAmbassador {
 public:
  void requestFederationRestoreSucceeded(std::wstring const& label) override {
    ++restoreSucceededCount;
    restoreLabel = label;
    callbackOrder.push_back("restore-request-succeeded");
  }

  void requestFederationRestoreFailed(std::wstring const& label) override {
    ++restoreFailedCount;
    restoreLabel = label;
    callbackOrder.push_back("restore-request-failed");
  }

  void initiateFederateSave(std::wstring const& label) override {
    ++saveInitiateCount;
    saveLabel = label;
  }

  void federationSaved() override { ++saveCompleteCount; }

  void federationNotSaved(rti1516_2025::SaveFailureReason reason) override {
    ++saveNotCompleteCount;
    saveFailureReason = reason;
  }

  void federationRestoreBegun() override {
    ++restoreBegunCount;
    callbackOrder.push_back("restore-begun");
  }

  void initiateFederateRestore(
      std::wstring const& label,
      std::wstring const& federateName,
      rti1516_2025::FederateHandle const& postRestoreFederateHandle) override {
    ++restoreInitiateCount;
    restoreLabel = label;
    restoreFederateName = federateName;
    restorePostFederateHandle = postRestoreFederateHandle;
    callbackOrder.push_back("restore-initiate");
  }

  void federationRestored() override {
    ++restoreCompleteCount;
    callbackOrder.push_back("restore-complete");
  }

  void federationNotRestored(rti1516_2025::RestoreFailureReason reason) override {
    ++restoreNotCompleteCount;
    restoreFailureReason = reason;
    callbackOrder.push_back("restore-failed");
  }

  void federationRestoreStatusResponse(
      rti1516_2025::FederateRestoreStatusVector const& response) override {
    federationRestoreStatusReports.push_back(response);
    callbackOrder.push_back("restore-status");
  }

  std::size_t restoreSucceededCount = 0U;
  std::size_t restoreFailedCount = 0U;
  std::size_t restoreBegunCount = 0U;
  std::size_t restoreInitiateCount = 0U;
  std::size_t restoreCompleteCount = 0U;
  std::size_t restoreNotCompleteCount = 0U;
  std::size_t saveInitiateCount = 0U;
  std::size_t saveCompleteCount = 0U;
  std::size_t saveNotCompleteCount = 0U;
  std::wstring restoreLabel;
  std::wstring saveLabel;
  std::wstring restoreFederateName;
  rti1516_2025::FederateHandle restorePostFederateHandle;
  rti1516_2025::SaveFailureReason saveFailureReason =
      rti1516_2025::SAVE_ABORTED;
  rti1516_2025::RestoreFailureReason restoreFailureReason =
      rti1516_2025::RTI_UNABLE_TO_RESTORE;
  std::vector<rti1516_2025::FederateRestoreStatusVector>
      federationRestoreStatusReports;
  std::vector<std::string> callbackOrder;
};

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FomCompositionStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::FomValidationStatus;
using umbra::detail::InteractionClassDeclarationStatus;
using umbra::detail::ObjectClassAttributeDeclarationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportConnection;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::ProcessTransportSession;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceMessageKind;
using umbra::detail::TransportServiceOperation;
using umbra::detail::TransportServiceStatus;

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

FederationDefinition composedDirectedProcessDefinition() {
  auto const objectConsumer =
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / "directed-interaction-object-consumer-fom.xml";
  auto const interactionProvider =
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / "directed-interaction-interaction-provider-fom.xml";
  std::vector<PrevalidatedFomModule> modules{
      validatedProcessModule(
          processResourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:directed-process-mim"),
      validatedProcessModule(
          objectConsumer,
          FomModuleKind::fom,
          L"urn:umbra:test:directed-process-object"),
      validatedProcessModule(
          interactionProvider,
          FomModuleKind::fom,
          L"urn:umbra:test:directed-process-interaction"),
  };
  LibXml2FomModuleComposer composer(
      processResourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid || !result.catalog ||
      !result.fdd) {
    throw std::runtime_error("The public directed process FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

#endif


std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path temporaryServiceReportDirectory() {
  static std::atomic_uint64_t next{0};
  // CTest starts separate processes whose counters all begin at zero. Claim
  // each directory atomically so parallel cases never share or remove it.
  for (;;) {
    auto const candidate = std::filesystem::temp_directory_path() /
        ("umbra-service-report-configuration-" + std::to_string(++next));
    if (std::filesystem::create_directory(candidate)) {
      return candidate;
    }
  }
}

void requireIgnoredConfiguration(ConfigurationResult const& result) {
  REQUIRE_FALSE(result.configurationUsed);
  REQUIRE_FALSE(result.addressUsed);
  REQUIRE(result.additionalSettingsResult == SETTINGS_IGNORED);
  REQUIRE(result.message.empty());
}

}  // namespace

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassadors expose defined GALT and LITS across configured process federates",
    "[integration][foundation][time-management][galt][lits][multi-federate][callback-immediate][transport][process-boundary][public-endpoint][rti.service.enable-time-regulation][rti.service.query-galt][rti.service.query-lits][federate.callback.time-regulation-enabled][2025]") {
  constexpr wchar_t const* federationName =
      L"process-time-bounds-multi-federate-execution";
  auto const serviceReportDirectory = temporaryServiceReportDirectory();

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions serviceOptions{true};
      serviceOptions.serviceReportDirectory = serviceReportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(serviceOptions));
      auto regulatorConnection = listener->accept(
          nullptr,
          {"process-time-bounds-multi-federate-server", 0x9213U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession regulator(regulatorConnection);
      auto regulatorHandler = service.handlerFor(regulator);
      auto serveExpected = [&](ProcessTransportSession& session,
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

      serveExpected(
          regulator,
          regulatorHandler,
          TransportServiceOperation::create_federation_execution,
          "The multi-federate time-bounds server lost Create.");
      serveExpected(
          regulator,
          regulatorHandler,
          TransportServiceOperation::join_federation_execution,
          "The multi-federate time-bounds server lost regulator Join.");
      serveExpected(
          regulator,
          regulatorHandler,
          TransportServiceOperation::enable_time_regulation,
          "The multi-federate time-bounds server lost Enable Time Regulation.");
      serveExpected(
          regulator,
          regulatorHandler,
          TransportServiceOperation::report_successful_void_service_invocation,
          "The multi-federate time-bounds server lost Enable Time Regulation reporting.");

      auto observerConnection = listener->accept(
          nullptr,
          {"process-time-bounds-multi-federate-server", 0x9214U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession observer(observerConnection);
      auto observerHandler = service.handlerFor(observer);
      serveExpected(
          observer,
          observerHandler,
          TransportServiceOperation::join_federation_execution,
          "The multi-federate time-bounds server lost observer Join.");
      for (auto* session : {&regulator, &observer}) {
        auto const& handler = session == &regulator ? regulatorHandler : observerHandler;
        serveExpected(
            *session,
            handler,
            TransportServiceOperation::set_service_reporting_switch,
            "The multi-federate time-bounds server lost service-report enablement.");
        serveExpected(
            *session,
            handler,
            TransportServiceOperation::set_send_service_reports_to_file_switch,
            "The multi-federate time-bounds server lost file-report enablement.");
      }
      serveExpected(
          regulator,
          regulatorHandler,
          TransportServiceOperation::query_lookahead,
          "The multi-federate time-bounds server lost regulator Query Lookahead.");
      serveExpected(
          regulator,
          regulatorHandler,
          TransportServiceOperation::report_successful_service_invocation,
          "The multi-federate time-bounds server lost regulator Query Lookahead reporting.");
      serveExpected(
          regulator,
          regulatorHandler,
          TransportServiceOperation::query_time_bounds,
          "The multi-federate time-bounds server lost regulator Query GALT.");
      serveExpected(
          regulator,
          regulatorHandler,
          TransportServiceOperation::report_successful_service_invocation,
          "The multi-federate time-bounds server lost regulator Query GALT reporting.");
      serveExpected(
          regulator,
          regulatorHandler,
          TransportServiceOperation::query_time_bounds,
          "The multi-federate time-bounds server lost regulator Query LITS.");
      serveExpected(
          regulator,
          regulatorHandler,
          TransportServiceOperation::report_successful_service_invocation,
          "The multi-federate time-bounds server lost regulator Query LITS reporting.");
      serveExpected(
          observer,
          observerHandler,
          TransportServiceOperation::query_time_bounds,
          "The multi-federate time-bounds server lost observer Query GALT.");
      serveExpected(
          observer,
          observerHandler,
          TransportServiceOperation::report_successful_service_invocation,
          "The multi-federate time-bounds server lost observer Query GALT reporting.");
      serveExpected(
          observer,
          observerHandler,
          TransportServiceOperation::query_time_bounds,
          "The multi-federate time-bounds server lost observer Query LITS.");
      serveExpected(
          observer,
          observerHandler,
          TransportServiceOperation::report_successful_service_invocation,
          "The multi-federate time-bounds server lost observer Query LITS reporting.");
      serveExpected(
          observer,
          observerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The multi-federate time-bounds server lost observer Resign.");
      serveExpected(
          regulator,
          regulatorHandler,
          TransportServiceOperation::resign_federation_execution,
          "The multi-federate time-bounds server lost regulator Resign.");
      service.detach(observer);
      service.detach(regulator);
      observerConnection->close();
      regulatorConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto regulator = makeRti();
  auto observer = makeRti();
  ProcessTimeFederateAmbassador regulatorFederate;
  ProcessTimeFederateAmbassador observerFederate;
  auto regulatorConfiguration = RtiConfiguration::createConfiguration()
                                    .withConfigurationName(
                                        L"process-time-bounds-multi-federate-regulator")
                                    .withRtiAddress(
                                        L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto observerConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"process-time-bounds-multi-federate-observer")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool regulatorJoined = false;
  bool observerJoined = false;
  try {
    auto const regulatorConnection = regulator->connect(
        regulatorFederate, HLA_IMMEDIATE, regulatorConfiguration);
    REQUIRE(regulatorConnection.addressUsed);
    REQUIRE_NOTHROW(regulator->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    auto const regulatorHandle = regulator->joinFederationExecution(
        L"process-time-bounds-multi-federate-regulator", federationName);
    REQUIRE(regulatorHandle.isValid());
    regulatorJoined = true;
    REQUIRE_NOTHROW(
        regulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE(regulatorFederate.timeRegulationEnabledCount == 1U);
    REQUIRE(
        regulatorFederate.timeRegulationEnabledImplementation ==
        L"HLAinteger64Time");

    auto const observerConnection = observer->connect(
        observerFederate, HLA_IMMEDIATE, observerConfiguration);
    REQUIRE(observerConnection.addressUsed);
    auto const observerHandle = observer->joinFederationExecution(
        L"process-time-bounds-multi-federate-observer", federationName);
    REQUIRE(observerHandle.isValid());
    REQUIRE(regulatorHandle != observerHandle);
    observerJoined = true;
    REQUIRE_NOTHROW(regulator->setServiceReportingSwitch(true));
    REQUIRE_NOTHROW(regulator->setSendServiceReportsToFileSwitch(true));
    REQUIRE_NOTHROW(observer->setServiceReportingSwitch(true));
    REQUIRE_NOTHROW(observer->setSendServiceReportsToFileSwitch(true));

    rti1516_2025::HLAinteger64Interval regulatorLookahead(99);
    REQUIRE_NOTHROW(regulator->queryLookahead(regulatorLookahead));
    REQUIRE(regulatorLookahead.getInterval() == 1);

    rti1516_2025::HLAinteger64Time regulatorGalt(99);
    REQUIRE_FALSE(regulator->queryGALT(regulatorGalt));
    REQUIRE(regulatorGalt.getTime() == 99);
    rti1516_2025::HLAinteger64Time regulatorLits(101);
    REQUIRE_FALSE(regulator->queryLITS(regulatorLits));
    REQUIRE(regulatorLits.getTime() == 101);

    rti1516_2025::HLAinteger64Time observerGalt(99);
    REQUIRE(observer->queryGALT(observerGalt));
    REQUIRE(observerGalt.getTime() == 1);
    rti1516_2025::HLAinteger64Time observerLits(101);
    REQUIRE(observer->queryLITS(observerLits));
    REQUIRE(observerLits.getTime() == 1);

    std::vector<std::string> queryGaltRecords;
    std::vector<std::string> queryLitsRecords;
    std::vector<std::string> queryLookaheadRecords;
    for (auto const& entry : std::filesystem::directory_iterator(serviceReportDirectory)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      std::ifstream reportFile(entry.path(), std::ios::binary);
      std::string contents{std::istreambuf_iterator<char>{reportFile},
                           std::istreambuf_iterator<char>{}};
      std::istringstream lines{contents};
      std::string line;
      while (std::getline(lines, line)) {
        if (line.find(R"("HLAservice":"QueryLookahead")") != std::string::npos) {
          queryLookaheadRecords.push_back(line);
        }
        if (line.find(R"("HLAservice":"QueryGALT")") != std::string::npos) {
          queryGaltRecords.push_back(line);
        }
        if (line.find(R"("HLAservice":"QueryLITS")") != std::string::npos) {
          queryLitsRecords.push_back(line);
        }
      }
    }
    REQUIRE(queryGaltRecords.size() == 2U);
    REQUIRE(queryLitsRecords.size() == 2U);
    REQUIRE(queryLookaheadRecords.size() == 1U);
    REQUIRE(std::any_of(
        queryGaltRecords.begin(), queryGaltRecords.end(), [](auto const& record) {
          return record.find(R"("HLAsuccessIndicator":true)") != std::string::npos &&
                 record.find(R"("HLAreturnedArgument":[{"HLAargumentType":34,"HLAargumentName":"","HLAargumentValue":null}])") != std::string::npos;
        }));
    REQUIRE(std::any_of(
        queryGaltRecords.begin(), queryGaltRecords.end(), [](auto const& record) {
          return record.find(R"("HLAsuccessIndicator":true)") != std::string::npos &&
                 record.find(R"("HLAreturnedArgument":[{"HLAargumentType":31,"HLAargumentName":"GALT","HLAargumentValue":"1"}])") != std::string::npos;
        }));
    REQUIRE(std::any_of(
        queryLitsRecords.begin(), queryLitsRecords.end(), [](auto const& record) {
          return record.find(R"("HLAsuccessIndicator":true)") != std::string::npos &&
                 record.find(R"("HLAreturnedArgument":[{"HLAargumentType":34,"HLAargumentName":"","HLAargumentValue":null}])") != std::string::npos;
        }));
    REQUIRE(std::any_of(
        queryLitsRecords.begin(), queryLitsRecords.end(), [](auto const& record) {
          return record.find(R"("HLAsuccessIndicator":true)") != std::string::npos &&
                 record.find(R"("HLAreturnedArgument":[{"HLAargumentType":31,"HLAargumentName":"LITS","HLAargumentValue":"1"}])") != std::string::npos;
        }));
    REQUIRE(std::any_of(
        queryLookaheadRecords.begin(), queryLookaheadRecords.end(), [](auto const& record) {
          return record.find(R"("HLAsuccessIndicator":true)") != std::string::npos &&
                 record.find(R"("HLAreturnedArgument":[{"HLAargumentType":32,"HLAargumentName":"Lookahead","HLAargumentValue":"1"}])") != std::string::npos;
        }));

    REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
    observerJoined = false;
    REQUIRE_NOTHROW(observer->disconnect());
    REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
    regulatorJoined = false;
    REQUIRE_NOTHROW(regulator->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (observerJoined) {
      try {
        observer->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (regulatorJoined) {
      try {
        regulator->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      observer->disconnect();
    } catch (...) {
    }
    try {
      regulator->disconnect();
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
}

TEST_CASE(
    "Process Time Constrained Enabled waits until GALT exceeds current logical time",
    "[integration][foundation][time-management][time-constrained][galt][process-boundary][public-endpoint][callback-immediate][callback-evoked][rti.service.enable-time-constrained][federate.callback.time-constrained-enabled][requirement:hla-1516.1-2025:clause-8.6.3][2025]") {
  auto const callbackModel = GENERATE(HLA_IMMEDIATE, HLA_EVOKED);
  auto const startWithSufficientGalt = GENERATE(false, true);
  constexpr wchar_t const* federationName =
      L"process-time-constrained-enable-galt-execution";
  auto const serviceReportDirectory = temporaryServiceReportDirectory();
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    std::vector<std::unique_ptr<ProcessTransportSession>> sessions;
    std::vector<std::thread> workers;
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options{true};
      options.serviceReportDirectory = serviceReportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
      std::array<umbra::detail::TransportEndpointIdentity, 2> identities{{
          {"process-time-constrained-galt-server", 0x9321U},
          {"process-time-constrained-galt-server", 0x9322U},
      }};
      for (auto const& identity : identities) {
        auto connection = listener->accept(
            nullptr, identity,
            [](std::wstring) {},
            [](std::wstring) { return false; });
        auto session = std::make_unique<ProcessTransportSession>(connection);
        auto* sessionPointer = session.get();
        auto handler = service.handlerFor(*sessionPointer);
        sessions.push_back(std::move(session));
        workers.emplace_back([&service, sessionPointer, handler, &serverError] {
          try {
            while (umbra::test::servePrimaryProcessRequest(
                *sessionPointer, handler)) {
            }
            service.detach(*sessionPointer);
          } catch (...) {
            serverError = std::current_exception();
          }
        });
      }
      for (auto& worker : workers) {
        worker.join();
      }
      for (auto& session : sessions) {
        if (session->connection()) {
          session->connection()->close();
        }
      }
    } catch (...) {
      serverError = std::current_exception();
      listener->close();
      for (auto& session : sessions) {
        if (session->connection()) {
          session->connection()->close();
        }
      }
      for (auto& worker : workers) {
        if (worker.joinable()) {
          worker.join();
        }
      }
    }
  });

  auto regulator = makeRti();
  auto target = makeRti();
  ProcessTimeFederateAmbassador regulatorFederate;
  ProcessConstrainedEnableWaitFederateAmbassador targetFederate;
  auto configurationFor = [port](std::wstring name) {
    return RtiConfiguration::createConfiguration()
        .withConfigurationName(std::move(name))
        .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
  };
  bool regulatorJoined = false;
  bool targetJoined = false;
  try {
    try {
      auto const connectResult = regulator->connect(
          regulatorFederate, HLA_IMMEDIATE,
          configurationFor(L"process-time-constrained-galt-regulator"));
      REQUIRE(connectResult.addressUsed);
    } catch (rti1516_2025::Exception const& error) {
      auto const message = error.name() + L": " + error.what();
      FAIL(std::string(message.begin(), message.end()));
    }
    REQUIRE_NOTHROW(regulator->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    REQUIRE(regulator->joinFederationExecution(
        L"process-time-constrained-galt-regulator", federationName)
                .isValid());
    regulatorJoined = true;
    REQUIRE_NOTHROW(regulator->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));

    REQUIRE(target->connect(
        targetFederate, callbackModel,
        configurationFor(L"process-time-constrained-galt-target"))
                .addressUsed);
    REQUIRE(target->joinFederationExecution(
        L"process-time-constrained-galt-target", federationName)
                .isValid());
    targetJoined = true;

    if (startWithSufficientGalt) {
      // Establish GALT=6 before the target asks to enable at current time 5.
      REQUIRE_NOTHROW(regulator->timeAdvanceRequest(
          rti1516_2025::HLAinteger64Time(5)));
    }

    // An unconstrained federate may advance independently. The generated
    // cases exercise both GALT below 5 and GALT already above 5.
    REQUIRE_NOTHROW(target->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    if (callbackModel == HLA_EVOKED) {
      auto const grantDeadline = std::chrono::steady_clock::now() +
                                 std::chrono::seconds(3);
      while (targetFederate.grantCount.load(std::memory_order_acquire) == 0U &&
             std::chrono::steady_clock::now() < grantDeadline) {
        static_cast<void>(target->evokeCallback(0.01));
      }
      REQUIRE(targetFederate.grantCount.load(std::memory_order_acquire) == 1U);
      REQUIRE(targetFederate.grantedTime.load(std::memory_order_acquire) == 5);
    }
    REQUIRE_NOTHROW(target->enableTimeConstrained());
    if (startWithSufficientGalt) {
      if (callbackModel == HLA_EVOKED) {
        auto const enableDeadline = std::chrono::steady_clock::now() +
                                    std::chrono::seconds(3);
        while (targetFederate.callbackCount.load(std::memory_order_acquire) == 0U &&
               std::chrono::steady_clock::now() < enableDeadline) {
          static_cast<void>(target->evokeCallback(0.01));
        }
      }
      REQUIRE(targetFederate.enabledTime.load(std::memory_order_acquire) == 5);
      REQUIRE(targetFederate.callbackCount.load(std::memory_order_acquire) == 1U);
      // Increasing GALT after an immediately eligible callback is one-shot.
      REQUIRE_NOTHROW(regulator->timeAdvanceRequest(
          rti1516_2025::HLAinteger64Time(6)));
      if (callbackModel == HLA_EVOKED) {
        REQUIRE_FALSE(target->evokeCallback(0.0));
      }
    } else {
      REQUIRE(targetFederate.callbackCount.load(std::memory_order_acquire) == 0U);
      if (callbackModel == HLA_EVOKED) {
        REQUIRE_FALSE(target->evokeCallback(0.0));
      }

      // Advancing the time-regulating peer to 4 moves GALT to 5, equal to the
      // target's current time. Equality is not enough: the pending role must
      // remain disabled until GALT advances beyond the current time.
      REQUIRE_NOTHROW(regulator->timeAdvanceRequest(
          rti1516_2025::HLAinteger64Time(4)));
      if (callbackModel == HLA_EVOKED) {
        REQUIRE_FALSE(target->evokeCallback(0.0));
      }
      REQUIRE(targetFederate.callbackCount.load(std::memory_order_acquire) == 0U);

      // Advancing the regulator to 5 moves GALT to 6. The RTI may now enable
      // time-constrained at the target's unchanged logical time 5.
      REQUIRE_NOTHROW(regulator->timeAdvanceRequest(
          rti1516_2025::HLAinteger64Time(5)));
      if (callbackModel == HLA_EVOKED) {
        auto const enableDeadline = std::chrono::steady_clock::now() +
                                   std::chrono::seconds(3);
        while (targetFederate.callbackCount.load(std::memory_order_acquire) == 0U &&
               std::chrono::steady_clock::now() < enableDeadline) {
          static_cast<void>(target->evokeCallback(0.01));
        }
        REQUIRE(targetFederate.callbackCount.load(std::memory_order_acquire) == 1U);
      } else {
        std::unique_lock lock(targetFederate.mutex);
        REQUIRE(targetFederate.changed.wait_for(
            lock, std::chrono::seconds(3), [&] {
              return targetFederate.callbackCount.load(
                         std::memory_order_acquire) == 1U;
            }));
      }
      REQUIRE(targetFederate.enabledTime.load(std::memory_order_acquire) == 5);
      REQUIRE(targetFederate.callbackCount.load(std::memory_order_acquire) == 1U);
      // Advancing GALT again must not enqueue a duplicate callback.
      REQUIRE_NOTHROW(regulator->timeAdvanceRequest(
          rti1516_2025::HLAinteger64Time(6)));
      if (callbackModel == HLA_EVOKED) {
        REQUIRE_FALSE(target->evokeCallback(0.0));
      }
      REQUIRE(targetFederate.callbackCount.load(std::memory_order_acquire) == 1U);
    }
  } catch (...) {
    auto const testFailure = std::current_exception();
    if (targetJoined) {
      try { target->resignFederationExecution(DELETE_OBJECTS); } catch (...) {}
    }
    if (regulatorJoined) {
      try { regulator->resignFederationExecution(DELETE_OBJECTS); } catch (...) {}
    }
    try { target->disconnect(); } catch (...) {}
    try { regulator->disconnect(); } catch (...) {}
    listener->close();
    server.join();
    std::rethrow_exception(testFailure);
  }
  REQUIRE_NOTHROW(target->resignFederationExecution(DELETE_OBJECTS));
  targetJoined = false;
  REQUIRE_NOTHROW(regulator->resignFederationExecution(DELETE_OBJECTS));
  regulatorJoined = false;
  target->disconnect();
  regulator->disconnect();
  server.join();
  if (serverError) {
    std::rethrow_exception(serverError);
  }
}
#endif
#endif
