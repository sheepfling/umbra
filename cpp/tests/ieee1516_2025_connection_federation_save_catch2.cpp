#include <catch2/catch_test_macros.hpp>

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
#include <string>
#include <thread>
#include <vector>
#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/auth/HLAnoCredentials.h>
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
    "RTIambassadors carry untimed federation save callbacks through the configured process endpoint",
    "[integration][foundation][federation-management][save-restore][transport][process-boundary][public-endpoint][2025][rti.service.request-federation-save][rti.service.federate-save-begun][rti.service.federate-save-complete][federate.callback.initiate-federate-save][federate.callback.federation-saved]") {
  using umbra::detail::ProcessTransportListener;

  constexpr wchar_t const* federationName =
      L"process-federation-save-execution";
  constexpr wchar_t const* federateType = L"process-federation-save-type";
  constexpr wchar_t const* saveLabel = L"process-federation-save-label";
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          composedProcessDefinition(),
          ProcessFederationServiceOptions{false});
      auto connection = listener->accept(
          nullptr,
          {"process-federation-save-server", 0x9261U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation,
                               char const* description) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(
                        std::string(description) +
                        " received an unexpected process operation.");
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };
      serveExpected(
          TransportServiceOperation::create_federation_execution,
          "The process federation-save server lost Create.");
      serveExpected(
          TransportServiceOperation::join_federation_execution,
          "The process federation-save server lost Join.");
      serveExpected(
          TransportServiceOperation::request_federation_save,
          "The process federation-save server lost Request Federation Save.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-save server lost save-initiation polling.");
      serveExpected(
          TransportServiceOperation::federate_save_begun,
          "The process federation-save server lost Federate Save Begun.");
      serveExpected(
          TransportServiceOperation::federate_save_complete,
          "The process federation-save server lost Federate Save Complete.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-save server lost save-completion polling.");
      serveExpected(
          TransportServiceOperation::resign_federation_execution,
          "The process federation-save server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessSaveFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-federation-save-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(
        federationName, L"server-owned-process-federation-save-fom.xml");
    REQUIRE(rti->joinFederationExecution(federateType, federationName)
                .isValid());

    rti->requestFederationSave(saveLabel);
    REQUIRE(federate.initiateCount == 0U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.initiateCount == 1U);
    REQUIRE(federate.initiateLabel == saveLabel);

    REQUIRE_NOTHROW(rti->federateSaveBegun());
    REQUIRE_NOTHROW(rti->federateSaveComplete());
    REQUIRE(federate.savedCount == 0U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.savedCount == 1U);
    REQUIRE(federate.notSavedCount == 0U);

    rti->resignFederationExecution(NO_ACTION);
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
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
    "RTIambassadors carry timestamped federation save callbacks through the configured process endpoint",
    "[integration][foundation][federation-management][save-restore][time-management][transport][process-boundary][public-endpoint][2025][rti.service.enable-time-regulation][rti.service.request-federation-save][rti.service.federate-save-begun][rti.service.federate-save-complete][federate.callback.time-regulation-enabled][federate.callback.initiate-federate-save][federate.callback.federation-saved][process-federation-save-timestamped]") {
  using umbra::detail::ProcessTransportListener;

  constexpr wchar_t const* federationName =
      L"process-timestamped-federation-save-execution";
  constexpr wchar_t const* federateType =
      L"process-timestamped-federation-save-type";
  constexpr wchar_t const* saveLabel =
      L"process-timestamped-federation-save-label";
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          composedProcessDefinition(),
          ProcessFederationServiceOptions{false});
      auto connection = listener->accept(
          nullptr,
          {"process-timestamped-federation-save-server", 0x9263U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation,
                               char const* description) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(
                        std::string(description) +
                        " received an unexpected process operation.");
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };
      serveExpected(
          TransportServiceOperation::create_federation_execution,
          "The process timestamped-save server lost Create.");
      serveExpected(
          TransportServiceOperation::join_federation_execution,
          "The process timestamped-save server lost Join.");
      serveExpected(
          TransportServiceOperation::enable_time_regulation,
          "The process timestamped-save server lost Enable Time Regulation.");
      serveExpected(
          TransportServiceOperation::request_federation_save,
          "The process timestamped-save server lost Request Federation Save.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process timestamped-save server lost save-initiation polling.");
      serveExpected(
          TransportServiceOperation::federate_save_begun,
          "The process timestamped-save server lost Federate Save Begun.");
      serveExpected(
          TransportServiceOperation::federate_save_complete,
          "The process timestamped-save server lost Federate Save Complete.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process timestamped-save server lost save-completion polling.");
      serveExpected(
          TransportServiceOperation::resign_federation_execution,
          "The process timestamped-save server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessSaveFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-timestamped-federation-save-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(
        federationName, L"server-owned-process-federation-save-fom.xml");
    REQUIRE(rti->joinFederationExecution(federateType, federationName)
                .isValid());

    REQUIRE_NOTHROW(
        rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.timeRegulationEnabledCount == 1U);

    rti1516_2025::HLAinteger64Time saveTime(5);
    REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel, saveTime));
    REQUIRE(federate.timedInitiateCount == 0U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.timedInitiateCount == 1U);
    REQUIRE(federate.timedInitiateLabel == saveLabel);
    REQUIRE(federate.timedInitiateImplementation == L"HLAinteger64Time");
    REQUIRE(federate.timedInitiateValue == saveTime.toString());

    REQUIRE_NOTHROW(rti->federateSaveBegun());
    REQUIRE_NOTHROW(rti->federateSaveComplete());
    REQUIRE(federate.savedCount == 0U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.savedCount == 1U);
    REQUIRE(federate.notSavedCount == 0U);

    rti->resignFederationExecution(NO_ACTION);
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
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
}

TEST_CASE(
    "RTIambassadors coordinate timestamped federation save boundaries through a configured process endpoint",
    "[integration][foundation][federation-management][save-restore][time-management][transport][process-boundary][public-endpoint][multi-federate][2025][timed-save][process-federation-save-timed-multi-federate][multi-federate-callback-ordering][rti.service.enable-time-regulation][rti.service.enable-time-constrained][rti.service.request-federation-save][rti.service.time-advance-request][rti.service.federate-save-begun][rti.service.federate-save-complete][federate.callback.time-regulation-enabled][federate.callback.time-constrained-enabled][federate.callback.time-advance-grant][federate.callback.initiate-federate-save][federate.callback.federation-saved]") {
  class MixedTimedSaveFederateAmbassador final : public NullFederateAmbassador {
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

    void timeAdvanceGrant(
        rti1516_2025::LogicalTime const& time) override {
      ++timeAdvanceGrantCount;
      timeAdvanceGrantImplementation = time.implementationName();
      timeAdvanceGrantValue = time.toString();
      callbackOrder.push_back("grant");
    }

    void initiateFederateSave(std::wstring const& label) override {
      ++untimedInitiateCount;
      untimedInitiateLabel = label;
      callbackOrder.push_back("save-initiate");
    }

    void initiateFederateSave(
        std::wstring const& label,
        rti1516_2025::LogicalTime const& time) override {
      ++timedInitiateCount;
      timedInitiateLabel = label;
      timedInitiateImplementation = time.implementationName();
      timedInitiateValue = time.toString();
      callbackOrder.push_back("save-initiate-timed");
    }

    void federationSaved() override {
      ++savedCount;
      callbackOrder.push_back("saved");
    }

    std::size_t timeRegulationEnabledCount = 0U;
    std::wstring timeRegulationEnabledImplementation;
    std::size_t timeConstrainedEnabledCount = 0U;
    std::wstring timeConstrainedEnabledImplementation;
    std::size_t timeAdvanceGrantCount = 0U;
    std::wstring timeAdvanceGrantImplementation;
    std::wstring timeAdvanceGrantValue;
    std::size_t untimedInitiateCount = 0U;
    std::wstring untimedInitiateLabel;
    std::size_t timedInitiateCount = 0U;
    std::wstring timedInitiateLabel;
    std::wstring timedInitiateImplementation;
    std::wstring timedInitiateValue;
    std::size_t savedCount = 0U;
    std::vector<std::string> callbackOrder;
  };

  using umbra::detail::ProcessTransportListener;

  constexpr wchar_t const* federationName =
      L"process-timed-federation-save-boundary-execution";
  constexpr wchar_t const* ownerType =
      L"process-timed-federation-save-owner-type";
  constexpr wchar_t const* receiverType =
      L"process-timed-federation-save-receiver-type";
  constexpr wchar_t const* saveLabel =
      L"process-timed-federation-save-boundary-label";
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          composedProcessDefinition(),
          ProcessFederationServiceOptions{false});
      auto ownerConnection = listener->accept(
          nullptr,
          {"process-timed-federation-save-boundary-server", 0x9264U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession ownerSession(ownerConnection);
      auto ownerHandler = service.handlerFor(ownerSession);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(
                        std::string(description) +
                        " received an unexpected process operation.");
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::create_federation_execution,
          "The timed process-save server lost Create.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::join_federation_execution,
          "The timed process-save server lost owner Join.");

      auto receiverConnection = listener->accept(
          nullptr,
          {"process-timed-federation-save-boundary-server", 0x9265U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiverSession(receiverConnection);
      auto receiverHandler = service.handlerFor(receiverSession);
      serveExpected(
          receiverSession,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The timed process-save server lost receiver Join.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::enable_time_regulation,
          "The timed process-save server lost owner Enable Time Regulation.");
      serveExpected(
          receiverSession,
          receiverHandler,
          TransportServiceOperation::enable_time_constrained,
          "The timed process-save server lost receiver Enable Time Constrained.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::request_federation_save,
          "The timed process-save server lost Request Federation Save.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::time_advance_request,
          "The timed process-save server lost owner TAR.");
      serveExpected(
          receiverSession,
          receiverHandler,
          TransportServiceOperation::time_advance_request,
          "The timed process-save server lost receiver TAR.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The timed process-save server lost owner save polling.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::federate_save_begun,
          "The timed process-save server lost owner Federate Save Begun.");
      serveExpected(
          receiverSession,
          receiverHandler,
          TransportServiceOperation::federate_save_begun,
          "The timed process-save server lost receiver Federate Save Begun.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::federate_save_complete,
          "The timed process-save server lost owner Federate Save Complete.");
      serveExpected(
          receiverSession,
          receiverHandler,
          TransportServiceOperation::federate_save_complete,
          "The timed process-save server lost receiver Federate Save Complete.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The timed process-save server lost owner completion polling.");
      serveExpected(
          receiverSession,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The timed process-save server lost receiver completion polling.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The timed process-save server lost owner Resign.");
      serveExpected(
          receiverSession,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The timed process-save server lost receiver Resign.");
      service.detach(receiverSession);
      service.detach(ownerSession);
      receiverConnection->close();
      ownerConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  MixedTimedSaveFederateAmbassador ownerFederate;
  MixedTimedSaveFederateAmbassador receiverFederate;
  auto ownerRti = makeRti();
  auto receiverRti = makeRti();
  auto ownerConfiguration = RtiConfiguration::createConfiguration()
                                .withConfigurationName(
                                    L"process-timed-federation-save-owner-client")
                                .withRtiAddress(
                                    L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto receiverConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"process-timed-federation-save-receiver-client")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool ownerJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(ownerRti->connect(ownerFederate, HLA_EVOKED, ownerConfiguration)
                .addressUsed);
    REQUIRE_NOTHROW(ownerRti->createFederationExecution(
        federationName,
        L"server-owned-process-timed-federation-save-fom.xml"));
    REQUIRE(ownerRti->joinFederationExecution(ownerType, federationName)
                .isValid());
    ownerJoined = true;

    REQUIRE(receiverRti->connect(
                receiverFederate,
                HLA_EVOKED,
                receiverConfiguration)
                .addressUsed);
    REQUIRE(receiverRti->joinFederationExecution(receiverType, federationName)
                .isValid());
    receiverJoined = true;

    REQUIRE_NOTHROW(ownerRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_FALSE(ownerRti->evokeCallback(0.0));
    REQUIRE(ownerFederate.timeRegulationEnabledCount == 1U);
    REQUIRE_NOTHROW(receiverRti->enableTimeConstrained());
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.timeConstrainedEnabledCount == 1U);

    rti1516_2025::HLAinteger64Time saveTime(5);
    REQUIRE_NOTHROW(ownerRti->requestFederationSave(saveLabel, saveTime));
    REQUIRE(ownerFederate.timedInitiateCount == 0U);
    REQUIRE(receiverFederate.timedInitiateCount == 0U);

    REQUIRE_NOTHROW(ownerRti->timeAdvanceRequest(saveTime));
    REQUIRE_FALSE(ownerRti->evokeCallback(0.0));
    REQUIRE(ownerFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(ownerFederate.timedInitiateCount == 0U);

    REQUIRE_NOTHROW(receiverRti->timeAdvanceRequest(saveTime));
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.timedInitiateCount == 1U);
    REQUIRE(receiverFederate.timedInitiateLabel == saveLabel);
    REQUIRE(receiverFederate.timedInitiateImplementation == L"HLAinteger64Time");
    REQUIRE(receiverFederate.timedInitiateValue == saveTime.toString());
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(ownerFederate.timedInitiateCount == 0U);
    REQUIRE_FALSE(ownerRti->evokeCallback(0.0));
    REQUIRE(ownerFederate.timedInitiateCount == 1U);
    REQUIRE(ownerFederate.timedInitiateLabel == saveLabel);
    REQUIRE(ownerFederate.timedInitiateImplementation == L"HLAinteger64Time");
    REQUIRE(ownerFederate.timedInitiateValue == saveTime.toString());

    REQUIRE_NOTHROW(ownerRti->federateSaveBegun());
    REQUIRE_NOTHROW(receiverRti->federateSaveBegun());
    REQUIRE_NOTHROW(ownerRti->federateSaveComplete());
    REQUIRE_NOTHROW(receiverRti->federateSaveComplete());
    REQUIRE_FALSE(ownerRti->evokeCallback(0.0));
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    REQUIRE(ownerFederate.savedCount == 1U);
    REQUIRE(receiverFederate.savedCount == 1U);

    REQUIRE_NOTHROW(ownerRti->resignFederationExecution(NO_ACTION));
    ownerJoined = false;
    REQUIRE_NOTHROW(receiverRti->resignFederationExecution(NO_ACTION));
    receiverJoined = false;
    REQUIRE_NOTHROW(ownerRti->disconnect());
    REQUIRE_NOTHROW(receiverRti->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (receiverJoined) {
      try {
        receiverRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (ownerJoined) {
      try {
        ownerRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      receiverRti->disconnect();
    } catch (...) {
    }
    try {
      ownerRti->disconnect();
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
    "RTIambassadors admit timestamped federation saves to multiple constrained recipients through a configured process endpoint",
    "[integration][foundation][federation-management][save-restore][time-management][transport][process-boundary][public-endpoint][multi-federate][multiple-constrained][2025][timed-save][multi-federate-callback-ordering][process-federation-save-timed-multiple-constrained][rti.service.enable-time-regulation][rti.service.enable-time-constrained][rti.service.request-federation-save][rti.service.time-advance-request][rti.service.federate-save-begun][rti.service.federate-save-complete][federate.callback.time-regulation-enabled][federate.callback.time-constrained-enabled][federate.callback.time-advance-grant][federate.callback.initiate-federate-save][federate.callback.federation-saved]") {
  class MultipleConstrainedTimedSaveFederateAmbassador final : public NullFederateAmbassador {
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

    void timeAdvanceGrant(
        rti1516_2025::LogicalTime const& time) override {
      ++timeAdvanceGrantCount;
      timeAdvanceGrantImplementation = time.implementationName();
      timeAdvanceGrantValue = time.toString();
      callbackOrder.push_back("grant");
    }

    void initiateFederateSave(
        std::wstring const& label,
        rti1516_2025::LogicalTime const& time) override {
      ++timedInitiateCount;
      timedInitiateLabel = label;
      timedInitiateImplementation = time.implementationName();
      timedInitiateValue = time.toString();
      callbackOrder.push_back("save-initiate-timed");
    }

    void federationSaved() override {
      ++savedCount;
      callbackOrder.push_back("saved");
    }

    std::size_t timeRegulationEnabledCount = 0U;
    std::wstring timeRegulationEnabledImplementation;
    std::size_t timeConstrainedEnabledCount = 0U;
    std::wstring timeConstrainedEnabledImplementation;
    std::size_t timeAdvanceGrantCount = 0U;
    std::wstring timeAdvanceGrantImplementation;
    std::wstring timeAdvanceGrantValue;
    std::size_t timedInitiateCount = 0U;
    std::wstring timedInitiateLabel;
    std::wstring timedInitiateImplementation;
    std::wstring timedInitiateValue;
    std::size_t savedCount = 0U;
    std::vector<std::string> callbackOrder;
  };

  using umbra::detail::ProcessTransportListener;

  constexpr wchar_t const* federationName =
      L"process-timed-federation-save-multiple-constrained-execution";
  constexpr wchar_t const* ownerType =
      L"process-timed-federation-save-multiple-owner-type";
  constexpr wchar_t const* firstType =
      L"process-timed-federation-save-multiple-first-type";
  constexpr wchar_t const* secondType =
      L"process-timed-federation-save-multiple-second-type";
  constexpr wchar_t const* saveLabel =
      L"process-timed-federation-save-multiple-label";
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          composedProcessDefinition(),
          ProcessFederationServiceOptions{false});
      auto ownerConnection = listener->accept(
          nullptr,
          {"process-timed-federation-save-multiple-server", 0x9266U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession ownerSession(ownerConnection);
      auto ownerHandler = service.handlerFor(ownerSession);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(
                        std::string(description) +
                        " received an unexpected process operation.");
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::create_federation_execution,
          "The multi-constrained timed process-save server lost Create.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::join_federation_execution,
          "The multi-constrained timed process-save server lost owner Join.");

      auto firstConnection = listener->accept(
          nullptr,
          {"process-timed-federation-save-multiple-server", 0x9267U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession firstSession(firstConnection);
      auto firstHandler = service.handlerFor(firstSession);
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::join_federation_execution,
          "The multi-constrained timed process-save server lost first Join.");

      auto secondConnection = listener->accept(
          nullptr,
          {"process-timed-federation-save-multiple-server", 0x9268U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession secondSession(secondConnection);
      auto secondHandler = service.handlerFor(secondSession);
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::join_federation_execution,
          "The multi-constrained timed process-save server lost second Join.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::enable_time_constrained,
          "The multi-constrained timed process-save server lost first Enable Time Constrained.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::enable_time_constrained,
          "The multi-constrained timed process-save server lost second Enable Time Constrained.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::enable_time_regulation,
          "The multi-constrained timed process-save server lost owner Enable Time Regulation.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::request_federation_save,
          "The multi-constrained timed process-save server lost Request Federation Save.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::time_advance_request,
          "The multi-constrained timed process-save server lost first TAR.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::time_advance_request,
          "The multi-constrained timed process-save server lost second TAR.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::time_advance_request,
          "The multi-constrained timed process-save server lost owner TAR.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-constrained timed process-save server lost owner save polling.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-constrained timed process-save server lost first save polling.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-constrained timed process-save server lost second save polling.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::federate_save_begun,
          "The multi-constrained timed process-save server lost owner Federate Save Begun.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::federate_save_begun,
          "The multi-constrained timed process-save server lost first Federate Save Begun.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::federate_save_begun,
          "The multi-constrained timed process-save server lost second Federate Save Begun.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::federate_save_complete,
          "The multi-constrained timed process-save server lost owner Federate Save Complete.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::federate_save_complete,
          "The multi-constrained timed process-save server lost first Federate Save Complete.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::federate_save_complete,
          "The multi-constrained timed process-save server lost second Federate Save Complete.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-constrained timed process-save server lost owner completion polling.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-constrained timed process-save server lost first completion polling.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-constrained timed process-save server lost second completion polling.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The multi-constrained timed process-save server lost owner Resign.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::resign_federation_execution,
          "The multi-constrained timed process-save server lost first Resign.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::resign_federation_execution,
          "The multi-constrained timed process-save server lost second Resign.");
      service.detach(secondSession);
      service.detach(firstSession);
      service.detach(ownerSession);
      secondConnection->close();
      firstConnection->close();
      ownerConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  MultipleConstrainedTimedSaveFederateAmbassador ownerFederate;
  MultipleConstrainedTimedSaveFederateAmbassador firstFederate;
  MultipleConstrainedTimedSaveFederateAmbassador secondFederate;
  auto ownerRti = makeRti();
  auto firstRti = makeRti();
  auto secondRti = makeRti();
  auto makeConfiguration = [&](wchar_t const* name) {
    return RtiConfiguration::createConfiguration()
        .withConfigurationName(name)
        .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
  };
  auto ownerConfiguration = makeConfiguration(
      L"process-timed-federation-save-multiple-owner-client");
  auto firstConfiguration = makeConfiguration(
      L"process-timed-federation-save-multiple-first-client");
  auto secondConfiguration = makeConfiguration(
      L"process-timed-federation-save-multiple-second-client");
  std::exception_ptr clientError;
  bool ownerJoined = false;
  bool firstJoined = false;
  bool secondJoined = false;
  try {
    REQUIRE(ownerRti->connect(ownerFederate, HLA_EVOKED, ownerConfiguration)
                .addressUsed);
    REQUIRE_NOTHROW(ownerRti->createFederationExecution(
        federationName,
        L"server-owned-process-timed-federation-save-fom.xml"));
    REQUIRE(ownerRti->joinFederationExecution(ownerType, federationName)
                .isValid());
    ownerJoined = true;

    REQUIRE(firstRti->connect(firstFederate, HLA_EVOKED, firstConfiguration)
                .addressUsed);
    REQUIRE(firstRti->joinFederationExecution(firstType, federationName)
                .isValid());
    firstJoined = true;

    REQUIRE(secondRti->connect(secondFederate, HLA_EVOKED, secondConfiguration)
                .addressUsed);
    REQUIRE(secondRti->joinFederationExecution(secondType, federationName)
                .isValid());
    secondJoined = true;

    REQUIRE_NOTHROW(firstRti->enableTimeConstrained());
    REQUIRE_FALSE(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.timeConstrainedEnabledCount == 1U);
    REQUIRE_NOTHROW(secondRti->enableTimeConstrained());
    REQUIRE_FALSE(secondRti->evokeCallback(0.0));
    REQUIRE(secondFederate.timeConstrainedEnabledCount == 1U);
    REQUIRE_NOTHROW(ownerRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_FALSE(ownerRti->evokeCallback(0.0));
    REQUIRE(ownerFederate.timeRegulationEnabledCount == 1U);

    rti1516_2025::HLAinteger64Time saveTime(5);
    REQUIRE_NOTHROW(ownerRti->requestFederationSave(saveLabel, saveTime));
    REQUIRE(ownerFederate.timedInitiateCount == 0U);
    REQUIRE(firstFederate.timedInitiateCount == 0U);
    REQUIRE(secondFederate.timedInitiateCount == 0U);

    REQUIRE_NOTHROW(firstRti->timeAdvanceRequest(saveTime));
    REQUIRE_NOTHROW(secondRti->timeAdvanceRequest(saveTime));
    REQUIRE_NOTHROW(ownerRti->timeAdvanceRequest(saveTime));
    REQUIRE_FALSE(ownerRti->evokeCallback(0.0));
    REQUIRE(ownerFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(ownerFederate.timedInitiateCount == 0U);

    static_cast<void>(ownerRti->evokeCallback(0.0));
    REQUIRE(ownerFederate.timedInitiateCount == 1U);
    REQUIRE(ownerFederate.timedInitiateLabel == saveLabel);
    REQUIRE(ownerFederate.timedInitiateImplementation == L"HLAinteger64Time");
    REQUIRE(ownerFederate.timedInitiateValue == saveTime.toString());
    REQUIRE(ownerFederate.callbackOrder ==
            std::vector<std::string>{"grant", "save-initiate-timed"});

    static_cast<void>(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.timedInitiateCount == 1U);
    REQUIRE(firstFederate.timedInitiateLabel == saveLabel);
    REQUIRE(firstFederate.timedInitiateImplementation == L"HLAinteger64Time");
    REQUIRE(firstFederate.timedInitiateValue == saveTime.toString());
    REQUIRE(firstFederate.timeAdvanceGrantCount == 0U);
    static_cast<void>(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(firstFederate.callbackOrder ==
            std::vector<std::string>{"save-initiate-timed", "grant"});

    static_cast<void>(secondRti->evokeCallback(0.0));
    REQUIRE(secondFederate.timedInitiateCount == 1U);
    REQUIRE(secondFederate.timedInitiateLabel == saveLabel);
    REQUIRE(secondFederate.timedInitiateImplementation == L"HLAinteger64Time");
    REQUIRE(secondFederate.timedInitiateValue == saveTime.toString());
    REQUIRE(secondFederate.timeAdvanceGrantCount == 0U);
    static_cast<void>(secondRti->evokeCallback(0.0));
    REQUIRE(secondFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(secondFederate.callbackOrder ==
            std::vector<std::string>{"save-initiate-timed", "grant"});

    REQUIRE_NOTHROW(ownerRti->federateSaveBegun());
    REQUIRE_NOTHROW(firstRti->federateSaveBegun());
    REQUIRE_NOTHROW(secondRti->federateSaveBegun());
    REQUIRE_NOTHROW(ownerRti->federateSaveComplete());
    REQUIRE_NOTHROW(firstRti->federateSaveComplete());
    REQUIRE_NOTHROW(secondRti->federateSaveComplete());
    static_cast<void>(ownerRti->evokeCallback(0.0));
    static_cast<void>(firstRti->evokeCallback(0.0));
    static_cast<void>(secondRti->evokeCallback(0.0));
    REQUIRE(ownerFederate.savedCount == 1U);
    REQUIRE(firstFederate.savedCount == 1U);
    REQUIRE(secondFederate.savedCount == 1U);

    REQUIRE_NOTHROW(ownerRti->resignFederationExecution(NO_ACTION));
    ownerJoined = false;
    REQUIRE_NOTHROW(firstRti->resignFederationExecution(NO_ACTION));
    firstJoined = false;
    REQUIRE_NOTHROW(secondRti->resignFederationExecution(NO_ACTION));
    secondJoined = false;
    REQUIRE_NOTHROW(ownerRti->disconnect());
    REQUIRE_NOTHROW(firstRti->disconnect());
    REQUIRE_NOTHROW(secondRti->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (secondJoined) {
      try {
        secondRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (firstJoined) {
      try {
        firstRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (ownerJoined) {
      try {
        ownerRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      secondRti->disconnect();
    } catch (...) {
    }
    try {
      firstRti->disconnect();
    } catch (...) {
    }
    try {
      ownerRti->disconnect();
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
    "RTIambassadors admit a timestamped federation save while queued TSO delivery remains pending for multiple constrained recipients through a configured process endpoint",
    "[integration][foundation][federation-management][interaction-management][save-restore][time-management][time-advance][transport][process-boundary][public-endpoint][multi-federate][multiple-constrained][queued-tso][2025][timed-save][multi-federate-callback-ordering][process-federation-save-timed-queued-tso][rti.service.enable-time-regulation][rti.service.enable-time-constrained][rti.service.send-interaction][rti.service.request-federation-save][rti.service.time-advance-request][rti.service.query-galt][rti.service.query-lits][rti.service.federate-save-begun][rti.service.federate-save-complete][federate.callback.receive-interaction][federate.callback.time-regulation-enabled][federate.callback.time-constrained-enabled][federate.callback.time-advance-grant][federate.callback.initiate-federate-save][federate.callback.federation-saved]") {
  class QueuedTsoTimedSaveFederateAmbassador final : public NullFederateAmbassador {
   public:
    void receiveInteraction(
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions,
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::OrderType sentOrder,
        rti1516_2025::OrderType receivedOrder,
        rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
      ++receivedInteractionCount;
      callbackOrder.push_back("interaction");
      receivedInteractionClass = interactionClass;
      receivedTransportationType = transportationType;
      receivedProducingFederate = producingFederate;
      receivedParameterCount = parameterValues.size();
      hasOptionalSentRegions = optionalSentRegions != nullptr;
      hasOptionalRetraction = optionalRetraction != nullptr;
      if (optionalRetraction != nullptr) {
        retractionIsValid = optionalRetraction->isValid();
      }
      sentOrderType = sentOrder;
      receivedOrderType = receivedOrder;
      receivedTimestampImplementation = time.implementationName();
      receivedTimestampValue = -1;
      if (auto const* integerTime =
              dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
        receivedTimestampValue = integerTime->getTime();
      }
      receivedTag.clear();
      if (userSuppliedTag.size() != 0U) {
        auto const* data =
            static_cast<std::uint8_t const*>(userSuppliedTag.data());
        receivedTag.assign(data, data + userSuppliedTag.size());
      }
    }

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

    void timeAdvanceGrant(
        rti1516_2025::LogicalTime const& time) override {
      ++timeAdvanceGrantCount;
      timeAdvanceGrantImplementation = time.implementationName();
      timeAdvanceGrantValue = time.toString();
      callbackOrder.push_back("grant");
    }

    void initiateFederateSave(
        std::wstring const& label,
        rti1516_2025::LogicalTime const& time) override {
      ++timedInitiateCount;
      timedInitiateLabel = label;
      timedInitiateImplementation = time.implementationName();
      timedInitiateValue = time.toString();
      callbackOrder.push_back("save-initiate-timed");
    }

    void federationSaved() override {
      ++savedCount;
      callbackOrder.push_back("saved");
    }

    std::size_t receivedInteractionCount = 0U;
    rti1516_2025::InteractionClassHandle receivedInteractionClass;
    rti1516_2025::TransportationTypeHandle receivedTransportationType;
    rti1516_2025::FederateHandle receivedProducingFederate;
    std::size_t receivedParameterCount = 0U;
    bool hasOptionalSentRegions = false;
    bool hasOptionalRetraction = false;
    bool retractionIsValid = false;
    rti1516_2025::OrderType sentOrderType = rti1516_2025::RECEIVE;
    rti1516_2025::OrderType receivedOrderType = rti1516_2025::RECEIVE;
    std::wstring receivedTimestampImplementation;
    std::int64_t receivedTimestampValue = -1;
    std::vector<std::uint8_t> receivedTag;
    std::size_t timeRegulationEnabledCount = 0U;
    std::wstring timeRegulationEnabledImplementation;
    std::size_t timeConstrainedEnabledCount = 0U;
    std::wstring timeConstrainedEnabledImplementation;
    std::size_t timeAdvanceGrantCount = 0U;
    std::wstring timeAdvanceGrantImplementation;
    std::wstring timeAdvanceGrantValue;
    std::size_t timedInitiateCount = 0U;
    std::wstring timedInitiateLabel;
    std::wstring timedInitiateImplementation;
    std::wstring timedInitiateValue;
    std::size_t savedCount = 0U;
    std::vector<std::string> callbackOrder;
  };

  using umbra::detail::ProcessTransportListener;

  constexpr wchar_t const* federationName =
      L"process-timed-federation-save-queued-tso-execution";
  constexpr wchar_t const* ownerType =
      L"process-timed-federation-save-queued-tso-owner-type";
  constexpr wchar_t const* firstType =
      L"process-timed-federation-save-queued-tso-first-type";
  constexpr wchar_t const* secondType =
      L"process-timed-federation-save-queued-tso-second-type";
  constexpr wchar_t const* saveLabel =
      L"process-timed-federation-save-queued-tso-label";
  constexpr char const* interactionName =
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr wchar_t const* interactionNameWide =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr char const* parameterName = "TimelinessOk";
  constexpr wchar_t const* parameterNameWide = L"TimelinessOk";

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::atomic_uint64_t expectedInteractionClass{0U};
  std::atomic_uint64_t expectedParameter{0U};
  std::atomic_uint64_t sendRecipientCount{0U};
  std::atomic_uint64_t sendMessageId{0U};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          composedProcessDefinition(),
          ProcessFederationServiceOptions{false});
      auto ownerConnection = listener->accept(
          nullptr,
          {"process-timed-federation-save-queued-tso-server", 0x9269U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession ownerSession(ownerConnection);
      auto ownerHandler = service.handlerFor(ownerSession);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(
                        std::string(description) +
                        " received an unexpected process operation.");
                  }
                  auto response = handler(request);
                  if (operation == TransportServiceOperation::send_interaction &&
                      response.status == TransportServiceStatus::ok) {
                    auto const result =
                        umbra::detail::decodeProcessFederationSendInteractionResult(
                            response.payload);
                    sendRecipientCount.store(
                        result.recipientCount, std::memory_order_release);
                    sendMessageId.store(
                        result.messageId, std::memory_order_release);
                  }
                  return response;
                })) {
          throw std::runtime_error(description);
        }
      };
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::create_federation_execution,
          "The queued-TSO timed-save server lost Create.");
      auto const interactionClass = registry.interactionClassHandleFor(
          federationName, interactionName);
      auto const parameter = registry.parameterHandleFor(
          federationName, interactionName, parameterName);
      if (!interactionClass || !parameter) {
        throw std::runtime_error(
            "The queued-TSO timed-save server could not resolve its FOM handles.");
      }
      expectedInteractionClass.store(*interactionClass, std::memory_order_release);
      expectedParameter.store(*parameter, std::memory_order_release);
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::join_federation_execution,
          "The queued-TSO timed-save server lost owner Join.");

      auto firstConnection = listener->accept(
          nullptr,
          {"process-timed-federation-save-queued-tso-server", 0x926AU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession firstSession(firstConnection);
      auto firstHandler = service.handlerFor(firstSession);
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::join_federation_execution,
          "The queued-TSO timed-save server lost first Join.");

      auto secondConnection = listener->accept(
          nullptr,
          {"process-timed-federation-save-queued-tso-server", 0x926BU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession secondSession(secondConnection);
      auto secondHandler = service.handlerFor(secondSession);
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::join_federation_execution,
          "The queued-TSO timed-save server lost second Join.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The queued-TSO timed-save server lost owner interaction lookup.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The queued-TSO timed-save server lost first interaction lookup.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The queued-TSO timed-save server lost second interaction lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::get_parameter_handle,
          "The queued-TSO timed-save server lost owner parameter lookup.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::publish_interaction_class,
          "The queued-TSO timed-save server lost Publish.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::subscribe_interaction_class,
          "The queued-TSO timed-save server lost first Subscribe.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::subscribe_interaction_class,
          "The queued-TSO timed-save server lost second Subscribe.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::enable_time_constrained,
          "The queued-TSO timed-save server lost first Enable Time Constrained.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::enable_time_constrained,
          "The queued-TSO timed-save server lost second Enable Time Constrained.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::enable_time_regulation,
          "The queued-TSO timed-save server lost owner Enable Time Regulation.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::send_interaction,
          "The queued-TSO timed-save server lost timestamped Send Interaction.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::request_federation_save,
          "The queued-TSO timed-save server lost Request Federation Save.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::time_advance_request,
          "The queued-TSO timed-save server lost first TAR(5).");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::time_advance_request,
          "The queued-TSO timed-save server lost second TAR(5).");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::time_advance_request,
          "The queued-TSO timed-save server lost owner TAR(5).");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The queued-TSO timed-save server lost owner save polling.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::receive_interaction,
          "The queued-TSO timed-save server lost first save polling.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::receive_interaction,
          "The queued-TSO timed-save server lost second save polling.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::federate_save_begun,
          "The queued-TSO timed-save server lost owner Federate Save Begun.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::federate_save_begun,
          "The queued-TSO timed-save server lost first Federate Save Begun.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::federate_save_begun,
          "The queued-TSO timed-save server lost second Federate Save Begun.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::federate_save_complete,
          "The queued-TSO timed-save server lost owner Federate Save Complete.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::federate_save_complete,
          "The queued-TSO timed-save server lost first Federate Save Complete.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::federate_save_complete,
          "The queued-TSO timed-save server lost second Federate Save Complete.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::receive_interaction,
          "The queued-TSO timed-save server lost owner completion polling.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::receive_interaction,
          "The queued-TSO timed-save server lost first completion polling.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::receive_interaction,
          "The queued-TSO timed-save server lost second completion polling.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::time_advance_request,
          "The queued-TSO timed-save server lost owner TAR(6).");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::time_advance_request,
          "The queued-TSO timed-save server lost first TAR(6).");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::query_time_bounds,
          "The queued-TSO timed-save server lost first Query GALT.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::query_time_bounds,
          "The queued-TSO timed-save server lost first Query LITS.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::acknowledge_tso_delivery,
          "The queued-TSO timed-save server lost first TSO acknowledgement.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::time_advance_request,
          "The queued-TSO timed-save server lost second TAR(6).");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::query_time_bounds,
          "The queued-TSO timed-save server lost second Query GALT.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::query_time_bounds,
          "The queued-TSO timed-save server lost second Query LITS.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::acknowledge_tso_delivery,
          "The queued-TSO timed-save server lost second TSO acknowledgement.");
      serveExpected(
          ownerSession,
          ownerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The queued-TSO timed-save server lost owner Resign.");
      serveExpected(
          firstSession,
          firstHandler,
          TransportServiceOperation::resign_federation_execution,
          "The queued-TSO timed-save server lost first Resign.");
      serveExpected(
          secondSession,
          secondHandler,
          TransportServiceOperation::resign_federation_execution,
          "The queued-TSO timed-save server lost second Resign.");
      service.detach(secondSession);
      service.detach(firstSession);
      service.detach(ownerSession);
      secondConnection->close();
      firstConnection->close();
      ownerConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  QueuedTsoTimedSaveFederateAmbassador ownerFederate;
  QueuedTsoTimedSaveFederateAmbassador firstFederate;
  QueuedTsoTimedSaveFederateAmbassador secondFederate;
  auto ownerRti = makeRti();
  auto firstRti = makeRti();
  auto secondRti = makeRti();
  auto makeConfiguration = [&](wchar_t const* name) {
    return RtiConfiguration::createConfiguration()
        .withConfigurationName(name)
        .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
  };
  auto ownerConfiguration = makeConfiguration(
      L"process-timed-federation-save-queued-tso-owner-client");
  auto firstConfiguration = makeConfiguration(
      L"process-timed-federation-save-queued-tso-first-client");
  auto secondConfiguration = makeConfiguration(
      L"process-timed-federation-save-queued-tso-second-client");
  std::exception_ptr clientError;
  bool ownerJoined = false;
  bool firstJoined = false;
  bool secondJoined = false;
  try {
    REQUIRE(ownerRti->connect(ownerFederate, HLA_EVOKED, ownerConfiguration)
                .addressUsed);
    REQUIRE_NOTHROW(ownerRti->createFederationExecution(
        federationName,
        L"server-owned-process-timed-federation-save-fom.xml"));
    REQUIRE(ownerRti->joinFederationExecution(ownerType, federationName)
                .isValid());
    ownerJoined = true;

    REQUIRE(firstRti->connect(firstFederate, HLA_EVOKED, firstConfiguration)
                .addressUsed);
    REQUIRE(firstRti->joinFederationExecution(firstType, federationName)
                .isValid());
    firstJoined = true;

    REQUIRE(secondRti->connect(secondFederate, HLA_EVOKED, secondConfiguration)
                .addressUsed);
    REQUIRE(secondRti->joinFederationExecution(secondType, federationName)
                .isValid());
    secondJoined = true;

    auto const ownerInteraction =
        ownerRti->getInteractionClassHandle(interactionNameWide);
    auto const firstInteraction =
        firstRti->getInteractionClassHandle(interactionNameWide);
    auto const secondInteraction =
        secondRti->getInteractionClassHandle(interactionNameWide);
    auto const expectedClass =
        rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
            expectedInteractionClass.load(std::memory_order_acquire));
    REQUIRE(ownerInteraction == expectedClass);
    REQUIRE(firstInteraction == expectedClass);
    REQUIRE(secondInteraction == expectedClass);
    auto const parameter = ownerRti->getParameterHandle(
        ownerInteraction, parameterNameWide);
    REQUIRE(parameter ==
            rti1516_2025::umbra_binding_detail::makeParameterHandle(
                expectedParameter.load(std::memory_order_acquire)));
    REQUIRE_NOTHROW(ownerRti->publishInteractionClass(ownerInteraction));
    REQUIRE_NOTHROW(firstRti->subscribeInteractionClass(firstInteraction, true));
    REQUIRE_NOTHROW(
        secondRti->subscribeInteractionClass(secondInteraction, true));

    REQUIRE_NOTHROW(firstRti->enableTimeConstrained());
    REQUIRE_FALSE(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.timeConstrainedEnabledCount == 1U);
    REQUIRE_NOTHROW(secondRti->enableTimeConstrained());
    REQUIRE_FALSE(secondRti->evokeCallback(0.0));
    REQUIRE(secondFederate.timeConstrainedEnabledCount == 1U);
    REQUIRE_NOTHROW(ownerRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_FALSE(ownerRti->evokeCallback(0.0));
    REQUIRE(ownerFederate.timeRegulationEnabledCount == 1U);

    rti1516_2025::HLAinteger64Time queuedTsoTime(6);
    rti1516_2025::HLAinteger64Time saveTime(5);
    std::array<std::uint8_t, 2U> encodedParameter{0x01U, 0x00U};
    ParameterHandleValueMap parameterValues;
    parameterValues.emplace(
        parameter,
        VariableLengthData(encodedParameter.data(), encodedParameter.size()));
    std::array<std::uint8_t, 3U> encodedTag{0x51U, 0x54U, 0x53U};
    VariableLengthData userSuppliedTag(encodedTag.data(), encodedTag.size());
    auto const retraction = ownerRti->sendInteraction(
        ownerInteraction,
        parameterValues,
        userSuppliedTag,
        queuedTsoTime);
    REQUIRE(retraction.isValid());
    REQUIRE(sendMessageId.load(std::memory_order_acquire) != 0U);
    REQUIRE(sendRecipientCount.load(std::memory_order_acquire) == 2U);

    REQUIRE_NOTHROW(ownerRti->requestFederationSave(saveLabel, saveTime));
    REQUIRE(ownerFederate.timedInitiateCount == 0U);
    REQUIRE(firstFederate.timedInitiateCount == 0U);
    REQUIRE(secondFederate.timedInitiateCount == 0U);

    REQUIRE_NOTHROW(firstRti->timeAdvanceRequest(saveTime));
    REQUIRE_NOTHROW(secondRti->timeAdvanceRequest(saveTime));
    REQUIRE_NOTHROW(ownerRti->timeAdvanceRequest(saveTime));
    REQUIRE_FALSE(ownerRti->evokeCallback(0.0));
    REQUIRE(ownerFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(ownerFederate.timedInitiateCount == 0U);

    static_cast<void>(ownerRti->evokeCallback(0.0));
    REQUIRE(ownerFederate.timedInitiateCount == 1U);
    REQUIRE(ownerFederate.timedInitiateLabel == saveLabel);
    REQUIRE(ownerFederate.timedInitiateImplementation == L"HLAinteger64Time");
    REQUIRE(ownerFederate.timedInitiateValue == saveTime.toString());
    REQUIRE(ownerFederate.receivedInteractionCount == 0U);
    REQUIRE(ownerFederate.callbackOrder ==
            std::vector<std::string>{"grant", "save-initiate-timed"});

    static_cast<void>(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.timedInitiateCount == 1U);
    REQUIRE(firstFederate.timedInitiateLabel == saveLabel);
    REQUIRE(firstFederate.timedInitiateImplementation == L"HLAinteger64Time");
    REQUIRE(firstFederate.timedInitiateValue == saveTime.toString());
    REQUIRE(firstFederate.receivedInteractionCount == 0U);
    REQUIRE(firstFederate.timeAdvanceGrantCount == 0U);
    static_cast<void>(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(firstFederate.receivedInteractionCount == 0U);
    REQUIRE(firstFederate.callbackOrder ==
            std::vector<std::string>{"save-initiate-timed", "grant"});

    static_cast<void>(secondRti->evokeCallback(0.0));
    REQUIRE(secondFederate.timedInitiateCount == 1U);
    REQUIRE(secondFederate.timedInitiateLabel == saveLabel);
    REQUIRE(secondFederate.timedInitiateImplementation == L"HLAinteger64Time");
    REQUIRE(secondFederate.timedInitiateValue == saveTime.toString());
    REQUIRE(secondFederate.receivedInteractionCount == 0U);
    REQUIRE(secondFederate.timeAdvanceGrantCount == 0U);
    static_cast<void>(secondRti->evokeCallback(0.0));
    REQUIRE(secondFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(secondFederate.receivedInteractionCount == 0U);
    REQUIRE(secondFederate.callbackOrder ==
            std::vector<std::string>{"save-initiate-timed", "grant"});

    REQUIRE_NOTHROW(ownerRti->federateSaveBegun());
    REQUIRE_NOTHROW(firstRti->federateSaveBegun());
    REQUIRE_NOTHROW(secondRti->federateSaveBegun());
    REQUIRE_NOTHROW(ownerRti->federateSaveComplete());
    REQUIRE_NOTHROW(firstRti->federateSaveComplete());
    REQUIRE_NOTHROW(secondRti->federateSaveComplete());
    static_cast<void>(ownerRti->evokeCallback(0.0));
    static_cast<void>(firstRti->evokeCallback(0.0));
    static_cast<void>(secondRti->evokeCallback(0.0));
    REQUIRE(ownerFederate.savedCount == 1U);
    REQUIRE(firstFederate.savedCount == 1U);
    REQUIRE(secondFederate.savedCount == 1U);
    REQUIRE(ownerFederate.receivedInteractionCount == 0U);
    REQUIRE(firstFederate.receivedInteractionCount == 0U);
    REQUIRE(secondFederate.receivedInteractionCount == 0U);

    REQUIRE_NOTHROW(ownerRti->timeAdvanceRequest(queuedTsoTime));
    REQUIRE_FALSE(ownerRti->evokeCallback(0.0));
    REQUIRE(ownerFederate.timeAdvanceGrantCount == 2U);

    REQUIRE_NOTHROW(firstRti->timeAdvanceRequest(queuedTsoTime));
    rti1516_2025::HLAinteger64Time firstGalt(99);
    REQUIRE(firstRti->queryGALT(firstGalt));
    REQUIRE(firstGalt.getTime() == 6);
    rti1516_2025::HLAinteger64Time firstLits(99);
    REQUIRE(firstRti->queryLITS(firstLits));
    REQUIRE(firstLits.getTime() == 6);
    static_cast<void>(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.receivedInteractionCount == 1U);
    REQUIRE(firstFederate.timeAdvanceGrantCount == 1U);
    static_cast<void>(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.timeAdvanceGrantCount == 2U);
    REQUIRE(firstFederate.receivedInteractionClass == expectedClass);
    REQUIRE(firstFederate.receivedProducingFederate.isValid());
    REQUIRE(firstFederate.receivedParameterCount == 1U);
    REQUIRE_FALSE(firstFederate.hasOptionalSentRegions);
    REQUIRE(firstFederate.hasOptionalRetraction);
    REQUIRE(firstFederate.retractionIsValid);
    REQUIRE(firstFederate.receivedTimestampImplementation ==
            L"HLAinteger64Time");
    REQUIRE(firstFederate.receivedTimestampValue == 6);
    REQUIRE(firstFederate.receivedTag ==
            std::vector<std::uint8_t>{0x51U, 0x54U, 0x53U});
    REQUIRE(firstFederate.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(firstFederate.receivedOrderType == rti1516_2025::TIMESTAMP);

    REQUIRE_NOTHROW(secondRti->timeAdvanceRequest(queuedTsoTime));
    rti1516_2025::HLAinteger64Time secondGalt(99);
    REQUIRE(secondRti->queryGALT(secondGalt));
    REQUIRE(secondGalt.getTime() == 6);
    rti1516_2025::HLAinteger64Time secondLits(99);
    REQUIRE(secondRti->queryLITS(secondLits));
    REQUIRE(secondLits.getTime() == 6);
    static_cast<void>(secondRti->evokeCallback(0.0));
    REQUIRE(secondFederate.receivedInteractionCount == 1U);
    REQUIRE(secondFederate.timeAdvanceGrantCount == 1U);
    static_cast<void>(secondRti->evokeCallback(0.0));
    REQUIRE(secondFederate.timeAdvanceGrantCount == 2U);
    REQUIRE(secondFederate.receivedInteractionClass == expectedClass);
    REQUIRE(secondFederate.receivedProducingFederate.isValid());
    REQUIRE(secondFederate.receivedParameterCount == 1U);
    REQUIRE_FALSE(secondFederate.hasOptionalSentRegions);
    REQUIRE(secondFederate.hasOptionalRetraction);
    REQUIRE(secondFederate.retractionIsValid);
    REQUIRE(secondFederate.receivedTimestampImplementation ==
            L"HLAinteger64Time");
    REQUIRE(secondFederate.receivedTimestampValue == 6);
    REQUIRE(secondFederate.receivedTag ==
            std::vector<std::uint8_t>{0x51U, 0x54U, 0x53U});
    REQUIRE(secondFederate.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(secondFederate.receivedOrderType == rti1516_2025::TIMESTAMP);

    REQUIRE(ownerFederate.callbackOrder ==
            std::vector<std::string>{
                "grant", "save-initiate-timed", "saved", "grant"});
    REQUIRE(firstFederate.callbackOrder ==
            std::vector<std::string>{
                "save-initiate-timed", "grant", "saved", "interaction", "grant"});
    REQUIRE(secondFederate.callbackOrder ==
            std::vector<std::string>{
                "save-initiate-timed", "grant", "saved", "interaction", "grant"});

    REQUIRE_NOTHROW(ownerRti->resignFederationExecution(NO_ACTION));
    ownerJoined = false;
    REQUIRE_NOTHROW(firstRti->resignFederationExecution(NO_ACTION));
    firstJoined = false;
    REQUIRE_NOTHROW(secondRti->resignFederationExecution(NO_ACTION));
    secondJoined = false;
    REQUIRE_NOTHROW(ownerRti->disconnect());
    REQUIRE_NOTHROW(firstRti->disconnect());
    REQUIRE_NOTHROW(secondRti->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (secondJoined) {
      try {
        secondRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (firstJoined) {
      try {
        firstRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (ownerJoined) {
      try {
        ownerRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      secondRti->disconnect();
    } catch (...) {
    }
    try {
      firstRti->disconnect();
    } catch (...) {
    }
    try {
      ownerRti->disconnect();
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
  REQUIRE(sendRecipientCount.load(std::memory_order_acquire) == 2U);
  REQUIRE(sendMessageId.load(std::memory_order_acquire) != 0U);
  REQUIRE_FALSE(ownerJoined);
  REQUIRE_FALSE(firstJoined);
  REQUIRE_FALSE(secondJoined);
}

TEST_CASE(
    "RTIambassadors carry federation save status responses through the configured process endpoint",
    "[integration][foundation][federation-management][save-restore][transport][process-boundary][public-endpoint][2025][rti.service.request-federation-save][rti.service.query-federation-save-status][rti.service.federate-save-begun][rti.service.federate-save-not-complete][federate.callback.initiate-federate-save][federate.callback.federation-save-status-response][federate.callback.federation-not-saved]") {
  using umbra::detail::ProcessTransportListener;

  constexpr wchar_t const* federationName =
      L"process-federation-save-status-execution";
  constexpr wchar_t const* federateType = L"process-federation-save-status-type";
  constexpr wchar_t const* saveLabel = L"process-federation-save-status-label";
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          composedProcessDefinition(),
          ProcessFederationServiceOptions{false});
      auto connection = listener->accept(
          nullptr,
          {"process-federation-save-status-server", 0x9262U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation,
                               char const* description) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(
                        std::string(description) +
                        " received an unexpected process operation.");
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };
      serveExpected(
          TransportServiceOperation::create_federation_execution,
          "The process federation-save-status server lost Create.");
      serveExpected(
          TransportServiceOperation::join_federation_execution,
          "The process federation-save-status server lost Join.");
      serveExpected(
          TransportServiceOperation::request_federation_save,
          "The process federation-save-status server lost Request Federation Save.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-save-status server lost save initiation polling.");
      serveExpected(
          TransportServiceOperation::query_federation_save_status,
          "The process federation-save-status server lost the first save-status query.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-save-status server lost the first status polling.");
      serveExpected(
          TransportServiceOperation::federate_save_begun,
          "The process federation-save-status server lost Federate Save Begun.");
      serveExpected(
          TransportServiceOperation::query_federation_save_status,
          "The process federation-save-status server lost the second save-status query.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-save-status server lost the second status polling.");
      serveExpected(
          TransportServiceOperation::federate_save_not_complete,
          "The process federation-save-status server lost Federate Save Not Complete.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-save-status server lost save-completion polling.");
      serveExpected(
          TransportServiceOperation::resign_federation_execution,
          "The process federation-save-status server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessSaveFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-federation-save-status-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(
        federationName, L"server-owned-process-federation-save-status-fom.xml");
    auto const federateHandle =
        rti->joinFederationExecution(federateType, federationName);
    REQUIRE(federateHandle.isValid());

    rti->requestFederationSave(saveLabel);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.initiateCount == 1U);
    REQUIRE(federate.initiateLabel == saveLabel);

    rti->queryFederationSaveStatus();
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.federationSaveStatusReports.size() == 1U);
    REQUIRE(federate.federationSaveStatusReports.back().size() == 1U);
    REQUIRE(federate.federationSaveStatusReports.back()[0].first ==
            federateHandle);
    REQUIRE(federate.federationSaveStatusReports.back()[0].second ==
            rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE);

    rti->federateSaveBegun();
    rti->queryFederationSaveStatus();
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.federationSaveStatusReports.size() == 2U);
    REQUIRE(federate.federationSaveStatusReports.back()[0].second ==
            rti1516_2025::FEDERATE_SAVING);

    rti->federateSaveNotComplete();
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.notSavedCount == 1U);
    REQUIRE(federate.notSavedReason ==
            rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_SAVE);

    rti->resignFederationExecution(NO_ACTION);
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
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
    "RTIambassadors carry federation save abort through the configured process endpoint",
    "[integration][foundation][federation-management][save-restore][transport][process-boundary][public-endpoint][2025][rti.service.request-federation-save][rti.service.abort-federation-save][federate.callback.initiate-federate-save][federate.callback.federation-not-saved]") {
  using umbra::detail::ProcessTransportListener;

  constexpr wchar_t const* federationName =
      L"process-federation-save-abort-execution";
  constexpr wchar_t const* federateType = L"process-federation-save-abort-type";
  constexpr wchar_t const* saveLabel = L"process-federation-save-abort-label";
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          composedProcessDefinition(),
          ProcessFederationServiceOptions{false});
      auto connection = listener->accept(
          nullptr,
          {"process-federation-save-abort-server", 0x9263U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation,
                               char const* description) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(
                        std::string(description) +
                        " received an unexpected process operation.");
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };
      serveExpected(
          TransportServiceOperation::create_federation_execution,
          "The process federation-save-abort server lost Create.");
      serveExpected(
          TransportServiceOperation::join_federation_execution,
          "The process federation-save-abort server lost Join.");
      serveExpected(
          TransportServiceOperation::request_federation_save,
          "The process federation-save-abort server lost Request Federation Save.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-save-abort server lost save initiation polling.");
      serveExpected(
          TransportServiceOperation::abort_federation_save,
          "The process federation-save-abort server lost Abort Federation Save.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-save-abort server lost save-abort polling.");
      serveExpected(
          TransportServiceOperation::resign_federation_execution,
          "The process federation-save-abort server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessSaveFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-federation-save-abort-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(
        federationName, L"server-owned-process-federation-save-abort-fom.xml");
    auto const federateHandle =
        rti->joinFederationExecution(federateType, federationName);
    REQUIRE(federateHandle.isValid());

    rti->requestFederationSave(saveLabel);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.initiateCount == 1U);
    REQUIRE(federate.initiateLabel == saveLabel);

    REQUIRE_NOTHROW(rti->abortFederationSave());
    REQUIRE(federate.notSavedCount == 0U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.notSavedCount == 1U);
    REQUIRE(federate.notSavedReason == rti1516_2025::SAVE_ABORTED);

    rti->resignFederationExecution(NO_ACTION);
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
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


#endif

#endif
