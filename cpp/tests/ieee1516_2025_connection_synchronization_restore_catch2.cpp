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
    "RTIambassadors carry federation synchronization points through the configured process endpoint",
    "[integration][foundation][federation-management][synchronization-point][transport][process-boundary][public-endpoint][multi-federate][2025]") {
  using umbra::detail::ProcessTransportListener;

  constexpr wchar_t const* federationName =
      L"process-synchronization-point-execution";
  constexpr wchar_t const* synchronizationLabel =
      L"process-synchronization-point";
  constexpr wchar_t const* federateType = L"process-synchronization-type";
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
      auto senderConnection = listener->accept(
          nullptr,
          {"process-synchronization-server", 0x9231U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderHandler = service.handlerFor(sender);
      auto serveExpected = [](
                                ProcessTransportSession& session,
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
          sender,
          senderHandler,
          TransportServiceOperation::create_federation_execution,
          "The process synchronization server lost Create.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The process synchronization server lost sender Join.");
      auto receiverConnection = listener->accept(
          nullptr,
          {"process-synchronization-server", 0x9232U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The process synchronization server lost receiver Join.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::register_federation_synchronization_point,
          "The process synchronization server lost synchronization registration.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::receive_interaction,
          "The process synchronization server lost sender announcement polling.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The process synchronization server lost receiver announcement polling.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::synchronization_point_achieved,
          "The process synchronization server lost sender achievement.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::synchronization_point_achieved,
          "The process synchronization server lost receiver achievement.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::receive_interaction,
          "The process synchronization server lost sender synchronized polling.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The process synchronization server lost receiver synchronized polling.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process synchronization server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process synchronization server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessSynchronizationFederateAmbassador senderFederate;
  ProcessSynchronizationFederateAmbassador receiverFederate;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto senderConfiguration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"process-synchronization-sender-client")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto receiverConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"process-synchronization-receiver-client")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  try {
    REQUIRE(senderRti->connect(
                senderFederate, HLA_EVOKED, senderConfiguration)
                .addressUsed);
    senderRti->createFederationExecution(
        federationName, L"server-owned-process-synchronization-fom.xml");
    REQUIRE(senderRti->joinFederationExecution(federateType, federationName)
                .isValid());

    REQUIRE(receiverRti->connect(
                receiverFederate, HLA_EVOKED, receiverConfiguration)
                .addressUsed);
    REQUIRE(receiverRti->joinFederationExecution(federateType, federationName)
                .isValid());

    std::array<std::uint8_t, 3U> tagBytes{0x11U, 0x22U, 0x33U};
    VariableLengthData tag;
    tag.setData(tagBytes.data(), tagBytes.size());
    senderRti->registerFederationSynchronizationPoint(
        synchronizationLabel, tag);
    REQUIRE(senderFederate.registrationSucceededCount == 0U);
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.registrationSucceededCount == 1U);
    REQUIRE(senderFederate.registrationLabel == synchronizationLabel);
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.announcementCount == 1U);
    REQUIRE(senderFederate.announcementLabel == synchronizationLabel);
    REQUIRE(senderFederate.announcementTag ==
            std::vector<std::uint8_t>(tagBytes.begin(), tagBytes.end()));
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.announcementCount == 1U);
    REQUIRE(receiverFederate.announcementLabel == synchronizationLabel);
    REQUIRE(receiverFederate.announcementTag ==
            std::vector<std::uint8_t>(tagBytes.begin(), tagBytes.end()));

    senderRti->synchronizationPointAchieved(synchronizationLabel);
    receiverRti->synchronizationPointAchieved(synchronizationLabel);
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.synchronizedCount == 1U);
    REQUIRE(senderFederate.synchronizedLabel == synchronizationLabel);
    REQUIRE(senderFederate.failedToSyncCount == 0U);
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.synchronizedCount == 1U);
    REQUIRE(receiverFederate.synchronizedLabel == synchronizationLabel);
    REQUIRE(receiverFederate.failedToSyncCount == 0U);

    senderRti->resignFederationExecution(NO_ACTION);
    receiverRti->resignFederationExecution(NO_ACTION);
    senderRti->disconnect();
    receiverRti->disconnect();
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
    "RTIambassadors carry federation restore request failures through the configured process endpoint",
    "[integration][foundation][federation-management][save-restore][transport][process-boundary][public-endpoint][2025][rti.service.request-federation-restore][federate.callback.request-federation-restore-failed]") {
  using umbra::detail::ProcessTransportListener;

  constexpr wchar_t const* federationName =
      L"process-federation-restore-request-execution";
  constexpr wchar_t const* federateType =
      L"process-federation-restore-request-type";
  constexpr wchar_t const* restoreLabel =
      L"process-federation-restore-missing-label";
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
          {"process-federation-restore-request-server", 0x9264U},
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
          "The process federation-restore server lost Create.");
      serveExpected(
          TransportServiceOperation::join_federation_execution,
          "The process federation-restore server lost Join.");
      serveExpected(
          TransportServiceOperation::request_federation_restore,
          "The process federation-restore server lost Request Federation Restore.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore server lost restore-failure polling.");
      serveExpected(
          TransportServiceOperation::resign_federation_execution,
          "The process federation-restore server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessRestoreFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-federation-restore-request-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(
        federationName,
        L"server-owned-process-federation-restore-request-fom.xml");
    auto const federateHandle =
        rti->joinFederationExecution(federateType, federationName);
    REQUIRE(federateHandle.isValid());

    REQUIRE_NOTHROW(rti->requestFederationRestore(restoreLabel));
    REQUIRE(federate.restoreFailedCount == 0U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreSucceededCount == 0U);
    REQUIRE(federate.restoreFailedCount == 1U);
    REQUIRE(federate.restoreLabel == restoreLabel);

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
    "RTIambassadors carry successful federation restore lifecycle through the configured process endpoint",
    "[integration][foundation][federation-management][save-restore][transport][process-boundary][public-endpoint][2025][rti.service.request-federation-save][rti.service.federate-save-begun][rti.service.federate-save-complete][rti.service.request-federation-restore][rti.service.federate-restore-complete][federate.callback.request-federation-restore-succeeded][federate.callback.federation-restore-begun][federate.callback.initiate-federate-restore][federate.callback.federation-restored]") {
  using umbra::detail::ProcessTransportListener;

  constexpr wchar_t const* federationName =
      L"process-federation-restore-success-execution";
  constexpr wchar_t const* federateType =
      L"process-federation-restore-success-type";
  constexpr wchar_t const* saveLabel =
      L"process-federation-restore-success-label";
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
          {"process-federation-restore-success-server", 0x9265U},
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
          "The process federation-restore-success server lost Create.");
      serveExpected(
          TransportServiceOperation::join_federation_execution,
          "The process federation-restore-success server lost Join.");
      serveExpected(
          TransportServiceOperation::request_federation_save,
          "The process federation-restore-success server lost Request Federation Save.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-success server lost save-initiation polling.");
      serveExpected(
          TransportServiceOperation::federate_save_begun,
          "The process federation-restore-success server lost Federate Save Begun.");
      serveExpected(
          TransportServiceOperation::federate_save_complete,
          "The process federation-restore-success server lost Federate Save Complete.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-success server lost save-completion polling.");
      serveExpected(
          TransportServiceOperation::request_federation_restore,
          "The process federation-restore-success server lost Request Federation Restore.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-success server lost restore-request polling.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-success server lost restore-begin polling.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-success server lost restore-initiation polling.");
      serveExpected(
          TransportServiceOperation::federate_restore_complete,
          "The process federation-restore-success server lost Federate Restore Complete.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-success server lost restore-completion polling.");
      serveExpected(
          TransportServiceOperation::resign_federation_execution,
          "The process federation-restore-success server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessRestoreFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-federation-restore-success-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(
        federationName,
        L"server-owned-process-federation-restore-success-fom.xml");
    auto const federateHandle =
        rti->joinFederationExecution(federateType, federationName);
    REQUIRE(federateHandle.isValid());

    REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.saveInitiateCount == 1U);
    REQUIRE(federate.saveLabel == saveLabel);
    REQUIRE_NOTHROW(rti->federateSaveBegun());
    REQUIRE_NOTHROW(rti->federateSaveComplete());
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.saveCompleteCount == 1U);

    REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
    REQUIRE(federate.restoreSucceededCount == 0U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreSucceededCount == 1U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreBegunCount == 1U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreInitiateCount == 1U);
    REQUIRE(federate.restoreLabel == saveLabel);
    REQUIRE_FALSE(federate.restoreFederateName.empty());
    REQUIRE(federate.restorePostFederateHandle.isValid());

    REQUIRE_NOTHROW(rti->federateRestoreComplete());
    REQUIRE(federate.restoreCompleteCount == 0U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreCompleteCount == 1U);
    REQUIRE(federate.restoreNotCompleteCount == 0U);
    REQUIRE(federate.callbackOrder ==
            std::vector<std::string>{
                "restore-request-succeeded",
                "restore-begun",
                "restore-initiate",
                "restore-complete"});

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
    "RTIambassadors carry federation restore failure lifecycle through the configured process endpoint",
    "[integration][foundation][federation-management][save-restore][transport][process-boundary][public-endpoint][2025][rti.service.request-federation-save][rti.service.federate-save-begun][rti.service.federate-save-complete][rti.service.request-federation-restore][rti.service.federate-restore-not-complete][federate.callback.request-federation-restore-succeeded][federate.callback.federation-restore-begun][federate.callback.initiate-federate-restore][federate.callback.federation-not-restored]") {
  using umbra::detail::ProcessTransportListener;

  constexpr wchar_t const* federationName =
      L"process-federation-restore-failure-execution";
  constexpr wchar_t const* federateType =
      L"process-federation-restore-failure-type";
  constexpr wchar_t const* saveLabel =
      L"process-federation-restore-failure-label";
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
          {"process-federation-restore-failure-server", 0x9266U},
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
          "The process federation-restore-failure server lost Create.");
      serveExpected(
          TransportServiceOperation::join_federation_execution,
          "The process federation-restore-failure server lost Join.");
      serveExpected(
          TransportServiceOperation::request_federation_save,
          "The process federation-restore-failure server lost Request Federation Save.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-failure server lost save-initiation polling.");
      serveExpected(
          TransportServiceOperation::federate_save_begun,
          "The process federation-restore-failure server lost Federate Save Begun.");
      serveExpected(
          TransportServiceOperation::federate_save_complete,
          "The process federation-restore-failure server lost Federate Save Complete.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-failure server lost save-completion polling.");
      serveExpected(
          TransportServiceOperation::request_federation_restore,
          "The process federation-restore-failure server lost Request Federation Restore.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-failure server lost restore-request polling.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-failure server lost restore-begin polling.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-failure server lost restore-initiation polling.");
      serveExpected(
          TransportServiceOperation::federate_restore_not_complete,
          "The process federation-restore-failure server lost Federate Restore Not Complete.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-failure server lost restore-failure polling.");
      serveExpected(
          TransportServiceOperation::resign_federation_execution,
          "The process federation-restore-failure server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessRestoreFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-federation-restore-failure-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(
        federationName,
        L"server-owned-process-federation-restore-failure-fom.xml");
    auto const federateHandle =
        rti->joinFederationExecution(federateType, federationName);
    REQUIRE(federateHandle.isValid());

    REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.saveInitiateCount == 1U);
    REQUIRE(federate.saveLabel == saveLabel);
    REQUIRE_NOTHROW(rti->federateSaveBegun());
    REQUIRE_NOTHROW(rti->federateSaveComplete());
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.saveCompleteCount == 1U);

    REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreSucceededCount == 1U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreBegunCount == 1U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreInitiateCount == 1U);
    REQUIRE(federate.restoreLabel == saveLabel);
    REQUIRE_FALSE(federate.restoreFederateName.empty());
    REQUIRE(federate.restorePostFederateHandle.isValid());

    REQUIRE_NOTHROW(rti->federateRestoreNotComplete());
    REQUIRE(federate.restoreNotCompleteCount == 0U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreNotCompleteCount == 1U);
    REQUIRE(federate.restoreCompleteCount == 0U);
    REQUIRE(federate.restoreFailureReason ==
            rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_RESTORE);
    REQUIRE(federate.callbackOrder ==
            std::vector<std::string>{
                "restore-request-succeeded",
                "restore-begun",
                "restore-initiate",
                "restore-failed"});

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
    "RTIambassadors carry federation restore abort through the configured process endpoint",
    "[integration][foundation][federation-management][save-restore][transport][process-boundary][public-endpoint][2025][rti.service.request-federation-save][rti.service.federate-save-begun][rti.service.federate-save-complete][rti.service.request-federation-restore][rti.service.abort-federation-restore][federate.callback.request-federation-restore-succeeded][federate.callback.federation-restore-begun][federate.callback.initiate-federate-restore][federate.callback.federation-not-restored]") {
  using umbra::detail::ProcessTransportListener;

  constexpr wchar_t const* federationName =
      L"process-federation-restore-abort-execution";
  constexpr wchar_t const* federateType =
      L"process-federation-restore-abort-type";
  constexpr wchar_t const* saveLabel =
      L"process-federation-restore-abort-label";
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
          {"process-federation-restore-abort-server", 0x9267U},
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
          "The process federation-restore-abort server lost Create.");
      serveExpected(
          TransportServiceOperation::join_federation_execution,
          "The process federation-restore-abort server lost Join.");
      serveExpected(
          TransportServiceOperation::request_federation_save,
          "The process federation-restore-abort server lost Request Federation Save.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-abort server lost save-initiation polling.");
      serveExpected(
          TransportServiceOperation::federate_save_begun,
          "The process federation-restore-abort server lost Federate Save Begun.");
      serveExpected(
          TransportServiceOperation::federate_save_complete,
          "The process federation-restore-abort server lost Federate Save Complete.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-abort server lost save-completion polling.");
      serveExpected(
          TransportServiceOperation::request_federation_restore,
          "The process federation-restore-abort server lost Request Federation Restore.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-abort server lost restore-request polling.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-abort server lost restore-begin polling.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-abort server lost restore-initiation polling.");
      serveExpected(
          TransportServiceOperation::abort_federation_restore,
          "The process federation-restore-abort server lost Abort Federation Restore.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-abort server lost restore-abort polling.");
      serveExpected(
          TransportServiceOperation::resign_federation_execution,
          "The process federation-restore-abort server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessRestoreFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-federation-restore-abort-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(
        federationName,
        L"server-owned-process-federation-restore-abort-fom.xml");
    auto const federateHandle =
        rti->joinFederationExecution(federateType, federationName);
    REQUIRE(federateHandle.isValid());

    REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.saveInitiateCount == 1U);
    REQUIRE(federate.saveLabel == saveLabel);
    REQUIRE_NOTHROW(rti->federateSaveBegun());
    REQUIRE_NOTHROW(rti->federateSaveComplete());
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.saveCompleteCount == 1U);

    REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreSucceededCount == 1U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreBegunCount == 1U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreInitiateCount == 1U);
    REQUIRE(federate.restoreLabel == saveLabel);
    REQUIRE_FALSE(federate.restoreFederateName.empty());
    REQUIRE(federate.restorePostFederateHandle.isValid());

    REQUIRE_NOTHROW(rti->abortFederationRestore());
    REQUIRE(federate.restoreNotCompleteCount == 0U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreNotCompleteCount == 1U);
    REQUIRE(federate.restoreCompleteCount == 0U);
    REQUIRE(federate.restoreFailureReason == rti1516_2025::RESTORE_ABORTED);
    REQUIRE(federate.callbackOrder ==
            std::vector<std::string>{
                "restore-request-succeeded",
                "restore-begun",
                "restore-initiate",
                "restore-failed"});

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
    "RTIambassadors carry federation restore status through the configured process endpoint",
    "[integration][foundation][federation-management][save-restore][transport][process-boundary][public-endpoint][2025][rti.service.request-federation-save][rti.service.federate-save-begun][rti.service.federate-save-complete][rti.service.request-federation-restore][rti.service.query-federation-restore-status][rti.service.federate-restore-complete][federate.callback.request-federation-restore-succeeded][federate.callback.federation-restore-begun][federate.callback.initiate-federate-restore][federate.callback.federation-restore-status-response][federate.callback.federation-restored]") {
  using umbra::detail::ProcessTransportListener;

  constexpr wchar_t const* federationName =
      L"process-federation-restore-status-execution";
  constexpr wchar_t const* federateType =
      L"process-federation-restore-status-type";
  constexpr wchar_t const* saveLabel =
      L"process-federation-restore-status-label";
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
          {"process-federation-restore-status-server", 0x9268U},
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
          "The process federation-restore-status server lost Create.");
      serveExpected(
          TransportServiceOperation::join_federation_execution,
          "The process federation-restore-status server lost Join.");
      serveExpected(
          TransportServiceOperation::request_federation_save,
          "The process federation-restore-status server lost Request Federation Save.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-status server lost save-initiation polling.");
      serveExpected(
          TransportServiceOperation::federate_save_begun,
          "The process federation-restore-status server lost Federate Save Begun.");
      serveExpected(
          TransportServiceOperation::federate_save_complete,
          "The process federation-restore-status server lost Federate Save Complete.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-status server lost save-completion polling.");
      serveExpected(
          TransportServiceOperation::request_federation_restore,
          "The process federation-restore-status server lost Request Federation Restore.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-status server lost restore-request polling.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-status server lost restore-begin polling.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-status server lost restore-initiation polling.");
      serveExpected(
          TransportServiceOperation::query_federation_restore_status,
          "The process federation-restore-status server lost Query Federation Restore Status.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-status server lost restore-status polling.");
      serveExpected(
          TransportServiceOperation::federate_restore_complete,
          "The process federation-restore-status server lost Federate Restore Complete.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-status server lost restore-completion polling.");
      serveExpected(
          TransportServiceOperation::resign_federation_execution,
          "The process federation-restore-status server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessRestoreFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-federation-restore-status-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(
        federationName,
        L"server-owned-process-federation-restore-status-fom.xml");
    auto const federateHandle =
        rti->joinFederationExecution(federateType, federationName);
    REQUIRE(federateHandle.isValid());

    REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.saveInitiateCount == 1U);
    REQUIRE(federate.saveLabel == saveLabel);
    REQUIRE_NOTHROW(rti->federateSaveBegun());
    REQUIRE_NOTHROW(rti->federateSaveComplete());
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.saveCompleteCount == 1U);

    REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreSucceededCount == 1U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreBegunCount == 1U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreInitiateCount == 1U);
    REQUIRE(federate.restorePostFederateHandle.isValid());

    REQUIRE_NOTHROW(rti->queryFederationRestoreStatus());
    REQUIRE(federate.federationRestoreStatusReports.empty());
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.federationRestoreStatusReports.size() == 1U);
    auto const& statuses = federate.federationRestoreStatusReports.back();
    REQUIRE(statuses.size() == 1U);
    REQUIRE(statuses.front().preRestoreHandle.isValid());
    REQUIRE(statuses.front().postRestoreHandle.isValid());
    REQUIRE(statuses.front().status == rti1516_2025::FEDERATE_RESTORING);

    REQUIRE_NOTHROW(rti->federateRestoreComplete());
    REQUIRE(federate.restoreCompleteCount == 0U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreCompleteCount == 1U);
    REQUIRE(federate.callbackOrder ==
            std::vector<std::string>{
                "restore-request-succeeded",
                "restore-begun",
                "restore-initiate",
                "restore-status",
                "restore-complete"});

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
    "RTIambassadors preserve idle and terminal federation restore status through the configured process endpoint",
    "[integration][foundation][federation-management][save-restore][transport][process-boundary][public-endpoint][2025][rti.service.query-federation-restore-status][rti.service.request-federation-save][rti.service.federate-save-begun][rti.service.federate-save-complete][rti.service.request-federation-restore][rti.service.federate-restore-complete][federate.callback.federation-restore-status-response][federate.callback.request-federation-restore-succeeded][federate.callback.federation-restore-begun][federate.callback.initiate-federate-restore][federate.callback.federation-restored]") {
  using umbra::detail::ProcessTransportListener;

  constexpr wchar_t const* federationName =
      L"process-federation-restore-status-boundaries-execution";
  constexpr wchar_t const* federateType =
      L"process-federation-restore-status-boundaries-type";
  constexpr wchar_t const* saveLabel =
      L"process-federation-restore-status-boundaries-label";
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
          {"process-federation-restore-status-boundaries-server", 0x9269U},
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
          "The process federation-restore-status-boundaries server lost Create.");
      serveExpected(
          TransportServiceOperation::join_federation_execution,
          "The process federation-restore-status-boundaries server lost Join.");
      serveExpected(
          TransportServiceOperation::query_federation_restore_status,
          "The process federation-restore-status-boundaries server lost idle Query Federation Restore Status.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-status-boundaries server lost idle-status polling.");
      serveExpected(
          TransportServiceOperation::request_federation_save,
          "The process federation-restore-status-boundaries server lost Request Federation Save.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-status-boundaries server lost save-initiation polling.");
      serveExpected(
          TransportServiceOperation::federate_save_begun,
          "The process federation-restore-status-boundaries server lost Federate Save Begun.");
      serveExpected(
          TransportServiceOperation::federate_save_complete,
          "The process federation-restore-status-boundaries server lost Federate Save Complete.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-status-boundaries server lost save-completion polling.");
      serveExpected(
          TransportServiceOperation::request_federation_restore,
          "The process federation-restore-status-boundaries server lost Request Federation Restore.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-status-boundaries server lost restore-request polling.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-status-boundaries server lost restore-begin polling.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-status-boundaries server lost restore-initiation polling.");
      serveExpected(
          TransportServiceOperation::query_federation_restore_status,
          "The process federation-restore-status-boundaries server lost in-progress Query Federation Restore Status.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-status-boundaries server lost in-progress-status polling.");
      serveExpected(
          TransportServiceOperation::federate_restore_complete,
          "The process federation-restore-status-boundaries server lost Federate Restore Complete.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-status-boundaries server lost restore-completion polling.");
      serveExpected(
          TransportServiceOperation::query_federation_restore_status,
          "The process federation-restore-status-boundaries server lost terminal Query Federation Restore Status.");
      serveExpected(
          TransportServiceOperation::receive_interaction,
          "The process federation-restore-status-boundaries server lost terminal-status polling.");
      serveExpected(
          TransportServiceOperation::resign_federation_execution,
          "The process federation-restore-status-boundaries server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessRestoreFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-federation-restore-status-boundaries-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(
        federationName,
        L"server-owned-process-federation-restore-status-boundaries-fom.xml");
    auto const federateHandle =
        rti->joinFederationExecution(federateType, federationName);
    REQUIRE(federateHandle.isValid());

    REQUIRE_NOTHROW(rti->queryFederationRestoreStatus());
    REQUIRE(federate.federationRestoreStatusReports.empty());
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.federationRestoreStatusReports.size() == 1U);
    auto const& idleStatuses = federate.federationRestoreStatusReports.back();
    REQUIRE(idleStatuses.size() == 1U);
    REQUIRE(idleStatuses.front().preRestoreHandle.isValid());
    REQUIRE_FALSE(idleStatuses.front().postRestoreHandle.isValid());
    REQUIRE(idleStatuses.front().status == rti1516_2025::NO_RESTORE_IN_PROGRESS);

    REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.saveInitiateCount == 1U);
    REQUIRE_NOTHROW(rti->federateSaveBegun());
    REQUIRE_NOTHROW(rti->federateSaveComplete());
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.saveCompleteCount == 1U);

    REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreSucceededCount == 1U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreBegunCount == 1U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreInitiateCount == 1U);

    REQUIRE_NOTHROW(rti->queryFederationRestoreStatus());
    REQUIRE(federate.federationRestoreStatusReports.size() == 1U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.federationRestoreStatusReports.size() == 2U);
    auto const& restoringStatuses = federate.federationRestoreStatusReports.back();
    REQUIRE(restoringStatuses.size() == 1U);
    REQUIRE(restoringStatuses.front().preRestoreHandle.isValid());
    REQUIRE(restoringStatuses.front().postRestoreHandle.isValid());
    REQUIRE(restoringStatuses.front().status == rti1516_2025::FEDERATE_RESTORING);

    REQUIRE_NOTHROW(rti->federateRestoreComplete());
    REQUIRE(federate.restoreCompleteCount == 0U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.restoreCompleteCount == 1U);

    REQUIRE_NOTHROW(rti->queryFederationRestoreStatus());
    REQUIRE(federate.federationRestoreStatusReports.size() == 2U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.federationRestoreStatusReports.size() == 3U);
    auto const& terminalStatuses = federate.federationRestoreStatusReports.back();
    REQUIRE(terminalStatuses.size() == 1U);
    REQUIRE(terminalStatuses.front().preRestoreHandle.isValid());
    REQUIRE_FALSE(terminalStatuses.front().postRestoreHandle.isValid());
    REQUIRE(terminalStatuses.front().status == rti1516_2025::NO_RESTORE_IN_PROGRESS);
    REQUIRE(federate.callbackOrder ==
            std::vector<std::string>{
                "restore-status",
                "restore-request-succeeded",
                "restore-begun",
                "restore-initiate",
                "restore-status",
                "restore-complete",
                "restore-status"});

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
    "RTIambassadors preserve multi-federate federation restore status projections through the configured process endpoint",
    "[integration][foundation][federation-management][save-restore][transport][process-boundary][public-endpoint][multi-federate][2025][rti.service.request-federation-save][rti.service.federate-save-begun][rti.service.federate-save-complete][rti.service.request-federation-restore][rti.service.query-federation-restore-status][rti.service.federate-restore-complete][federate.callback.request-federation-restore-succeeded][federate.callback.federation-restore-begun][federate.callback.initiate-federate-restore][federate.callback.federation-restore-status-response][federate.callback.federation-restored]") {
  using umbra::detail::ProcessTransportListener;

  constexpr wchar_t const* federationName =
      L"process-federation-restore-status-multi-federate-execution";
  constexpr wchar_t const* firstFederateType =
      L"process-federation-restore-status-multi-federate-first-type";
  constexpr wchar_t const* secondFederateType =
      L"process-federation-restore-status-multi-federate-second-type";
  constexpr wchar_t const* saveLabel =
      L"process-federation-restore-status-multi-federate-label";
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
      auto firstConnection = listener->accept(
          nullptr,
          {"process-federation-restore-status-multi-federate-server", 0x926AU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession first(firstConnection);
      auto firstHandler = service.handlerFor(first);
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
          first,
          firstHandler,
          TransportServiceOperation::create_federation_execution,
          "The multi-federate restore-status server lost Create.");
      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::join_federation_execution,
          "The multi-federate restore-status server lost first Join.");

      auto secondConnection = listener->accept(
          nullptr,
          {"process-federation-restore-status-multi-federate-server", 0x926BU},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession second(secondConnection);
      auto secondHandler = service.handlerFor(second);
      serveExpected(
          second,
          secondHandler,
          TransportServiceOperation::join_federation_execution,
          "The multi-federate restore-status server lost second Join.");

      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::request_federation_save,
          "The multi-federate restore-status server lost Request Federation Save.");
      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-federate restore-status server lost first save initiation polling.");
      serveExpected(
          second,
          secondHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-federate restore-status server lost second save initiation polling.");
      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::federate_save_begun,
          "The multi-federate restore-status server lost first Federate Save Begun.");
      serveExpected(
          second,
          secondHandler,
          TransportServiceOperation::federate_save_begun,
          "The multi-federate restore-status server lost second Federate Save Begun.");
      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::federate_save_complete,
          "The multi-federate restore-status server lost first Federate Save Complete.");
      serveExpected(
          second,
          secondHandler,
          TransportServiceOperation::federate_save_complete,
          "The multi-federate restore-status server lost second Federate Save Complete.");
      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-federate restore-status server lost first save completion polling.");
      serveExpected(
          second,
          secondHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-federate restore-status server lost second save completion polling.");

      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::request_federation_restore,
          "The multi-federate restore-status server lost Request Federation Restore.");
      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-federate restore-status server lost first restore success polling.");
      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-federate restore-status server lost first restore-begun polling.");
      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-federate restore-status server lost first restore-initiation polling.");
      serveExpected(
          second,
          secondHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-federate restore-status server lost second restore-begun polling.");
      serveExpected(
          second,
          secondHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-federate restore-status server lost second restore-initiation polling.");

      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::query_federation_restore_status,
          "The multi-federate restore-status server lost first in-progress Query Federation Restore Status.");
      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-federate restore-status server lost first in-progress status polling.");
      serveExpected(
          second,
          secondHandler,
          TransportServiceOperation::query_federation_restore_status,
          "The multi-federate restore-status server lost second in-progress Query Federation Restore Status.");
      serveExpected(
          second,
          secondHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-federate restore-status server lost second in-progress status polling.");
      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::federate_restore_complete,
          "The multi-federate restore-status server lost first Federate Restore Complete.");
      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::query_federation_restore_status,
          "The multi-federate restore-status server lost waiting Query Federation Restore Status.");
      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-federate restore-status server lost waiting status polling.");
      serveExpected(
          second,
          secondHandler,
          TransportServiceOperation::query_federation_restore_status,
          "The multi-federate restore-status server lost second waiting Query Federation Restore Status.");
      serveExpected(
          second,
          secondHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-federate restore-status server lost second waiting status polling.");
      serveExpected(
          second,
          secondHandler,
          TransportServiceOperation::federate_restore_complete,
          "The multi-federate restore-status server lost second Federate Restore Complete.");
      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-federate restore-status server lost first restore completion polling.");
      serveExpected(
          second,
          secondHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-federate restore-status server lost second restore completion polling.");
      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::query_federation_restore_status,
          "The multi-federate restore-status server lost terminal Query Federation Restore Status.");
      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::receive_interaction,
          "The multi-federate restore-status server lost terminal status polling.");

      serveExpected(
          second,
          secondHandler,
          TransportServiceOperation::resign_federation_execution,
          "The multi-federate restore-status server lost second Resign.");
      serveExpected(
          first,
          firstHandler,
          TransportServiceOperation::resign_federation_execution,
          "The multi-federate restore-status server lost first Resign.");
      service.detach(second);
      service.detach(first);
      secondConnection->close();
      firstConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto firstRti = makeRti();
  auto secondRti = makeRti();
  ProcessRestoreFederateAmbassador firstFederate;
  ProcessRestoreFederateAmbassador secondFederate;
  auto firstConfiguration = RtiConfiguration::createConfiguration()
                                .withConfigurationName(
                                    L"process-federation-restore-status-multi-federate-first")
                                .withRtiAddress(
                                    L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto secondConfiguration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"process-federation-restore-status-multi-federate-second")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool firstJoined = false;
  bool secondJoined = false;
  try {
    REQUIRE(firstRti->connect(firstFederate, HLA_EVOKED, firstConfiguration)
                .addressUsed);
    REQUIRE_NOTHROW(firstRti->createFederationExecution(
        federationName,
        L"server-owned-process-federation-restore-status-fom.xml"));
    auto const firstHandle = firstRti->joinFederationExecution(
        firstFederateType,
        federationName);
    REQUIRE(firstHandle.isValid());
    firstJoined = true;

    REQUIRE(secondRti->connect(secondFederate, HLA_EVOKED, secondConfiguration)
                .addressUsed);
    auto const secondHandle = secondRti->joinFederationExecution(
        secondFederateType,
        federationName);
    REQUIRE(secondHandle.isValid());
    REQUIRE(firstHandle != secondHandle);
    secondJoined = true;

    REQUIRE_NOTHROW(firstRti->requestFederationSave(saveLabel));
    REQUIRE_FALSE(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.saveInitiateCount == 1U);
    REQUIRE_FALSE(secondRti->evokeCallback(0.0));
    REQUIRE(secondFederate.saveInitiateCount == 1U);
    REQUIRE_NOTHROW(firstRti->federateSaveBegun());
    REQUIRE_NOTHROW(secondRti->federateSaveBegun());
    REQUIRE_NOTHROW(firstRti->federateSaveComplete());
    REQUIRE_NOTHROW(secondRti->federateSaveComplete());
    REQUIRE_FALSE(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.saveCompleteCount == 1U);
    REQUIRE_FALSE(secondRti->evokeCallback(0.0));
    REQUIRE(secondFederate.saveCompleteCount == 1U);

    REQUIRE_NOTHROW(firstRti->requestFederationRestore(saveLabel));
    REQUIRE_FALSE(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.restoreSucceededCount == 1U);
    REQUIRE_FALSE(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.restoreBegunCount == 1U);
    REQUIRE_FALSE(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.restoreInitiateCount == 1U);
    REQUIRE_FALSE(secondRti->evokeCallback(0.0));
    REQUIRE(secondFederate.restoreBegunCount == 1U);
    REQUIRE_FALSE(secondRti->evokeCallback(0.0));
    REQUIRE(secondFederate.restoreInitiateCount == 1U);

    auto requireStatus = [](auto const& statuses,
                            auto const& handle,
                            rti1516_2025::RestoreStatus expectedStatus,
                            bool expectedPostHandleValidity) {
      auto const status = std::find_if(
          statuses.begin(),
          statuses.end(),
          [&handle](auto const& candidate) {
            return candidate.preRestoreHandle == handle;
          });
      REQUIRE(status != statuses.end());
      if (status == statuses.end()) {
        return;
      }
      REQUIRE(status->status == expectedStatus);
      REQUIRE(status->postRestoreHandle.isValid() == expectedPostHandleValidity);
      if (expectedPostHandleValidity) {
        REQUIRE(status->postRestoreHandle == handle);
      }
    };

    REQUIRE_NOTHROW(firstRti->queryFederationRestoreStatus());
    REQUIRE(firstFederate.federationRestoreStatusReports.empty());
    REQUIRE_FALSE(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.federationRestoreStatusReports.size() == 1U);
    auto const& firstRestoringStatuses =
        firstFederate.federationRestoreStatusReports.back();
    REQUIRE(firstRestoringStatuses.size() == 2U);
    requireStatus(
        firstRestoringStatuses,
        firstHandle,
        rti1516_2025::FEDERATE_RESTORING,
        true);
    requireStatus(
        firstRestoringStatuses,
        secondHandle,
        rti1516_2025::FEDERATE_RESTORING,
        true);

    REQUIRE_NOTHROW(secondRti->queryFederationRestoreStatus());
    REQUIRE(secondFederate.federationRestoreStatusReports.empty());
    REQUIRE_FALSE(secondRti->evokeCallback(0.0));
    REQUIRE(secondFederate.federationRestoreStatusReports.size() == 1U);
    auto const& secondRestoringStatuses =
        secondFederate.federationRestoreStatusReports.back();
    REQUIRE(secondRestoringStatuses.size() == 2U);
    requireStatus(
        secondRestoringStatuses,
        firstHandle,
        rti1516_2025::FEDERATE_RESTORING,
        true);
    requireStatus(
        secondRestoringStatuses,
        secondHandle,
        rti1516_2025::FEDERATE_RESTORING,
        true);

    REQUIRE_NOTHROW(firstRti->federateRestoreComplete());
    REQUIRE(firstFederate.restoreCompleteCount == 0U);
    REQUIRE_NOTHROW(firstRti->queryFederationRestoreStatus());
    REQUIRE_FALSE(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.federationRestoreStatusReports.size() == 2U);
    auto const& firstWaitingStatuses =
        firstFederate.federationRestoreStatusReports.back();
    REQUIRE(firstWaitingStatuses.size() == 2U);
    requireStatus(
        firstWaitingStatuses,
        firstHandle,
        rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE,
        true);
    requireStatus(
        firstWaitingStatuses,
        secondHandle,
        rti1516_2025::FEDERATE_RESTORING,
        true);

    REQUIRE_NOTHROW(secondRti->queryFederationRestoreStatus());
    REQUIRE_FALSE(secondRti->evokeCallback(0.0));
    REQUIRE(secondFederate.federationRestoreStatusReports.size() == 2U);
    auto const& secondWaitingStatuses =
        secondFederate.federationRestoreStatusReports.back();
    REQUIRE(secondWaitingStatuses.size() == 2U);
    requireStatus(
        secondWaitingStatuses,
        firstHandle,
        rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE,
        true);
    requireStatus(
        secondWaitingStatuses,
        secondHandle,
        rti1516_2025::FEDERATE_RESTORING,
        true);

    REQUIRE_NOTHROW(secondRti->federateRestoreComplete());
    REQUIRE_FALSE(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.restoreCompleteCount == 1U);
    REQUIRE_FALSE(secondRti->evokeCallback(0.0));
    REQUIRE(secondFederate.restoreCompleteCount == 1U);

    REQUIRE_NOTHROW(firstRti->queryFederationRestoreStatus());
    REQUIRE_FALSE(firstRti->evokeCallback(0.0));
    REQUIRE(firstFederate.federationRestoreStatusReports.size() == 3U);
    auto const& firstTerminalStatuses =
        firstFederate.federationRestoreStatusReports.back();
    REQUIRE(firstTerminalStatuses.size() == 2U);
    requireStatus(
        firstTerminalStatuses,
        firstHandle,
        rti1516_2025::NO_RESTORE_IN_PROGRESS,
        false);
    requireStatus(
        firstTerminalStatuses,
        secondHandle,
        rti1516_2025::NO_RESTORE_IN_PROGRESS,
        false);
    REQUIRE(firstFederate.callbackOrder ==
            std::vector<std::string>{
                "restore-request-succeeded",
                "restore-begun",
                "restore-initiate",
                "restore-status",
                "restore-status",
                "restore-complete",
                "restore-status"});
    REQUIRE(secondFederate.callbackOrder ==
            std::vector<std::string>{
                "restore-begun",
                "restore-initiate",
                "restore-status",
                "restore-status",
                "restore-complete"});

    REQUIRE_NOTHROW(secondRti->resignFederationExecution(NO_ACTION));
    secondJoined = false;
    REQUIRE_NOTHROW(secondRti->disconnect());
    REQUIRE_NOTHROW(firstRti->resignFederationExecution(NO_ACTION));
    firstJoined = false;
    REQUIRE_NOTHROW(firstRti->disconnect());
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
    try {
      secondRti->disconnect();
    } catch (...) {
    }
    try {
      firstRti->disconnect();
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


#endif

#endif
