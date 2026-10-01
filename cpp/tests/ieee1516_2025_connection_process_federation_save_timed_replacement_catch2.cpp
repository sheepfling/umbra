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
    "RTIambassadors apply timestamped federation-save replacement through a configured process endpoint",
    "[integration][foundation][federation-management][save-restore][time-management][transport][process-boundary][public-endpoint][2025][timed-save][save-replacement][process-federation-save-timed-replacement][multi-federate-callback-ordering][rti.service.enable-time-regulation][rti.service.enable-time-constrained][rti.service.request-federation-save][rti.service.time-advance-request][rti.service.federate-save-begun][rti.service.federate-save-complete][federate.callback.time-regulation-enabled][federate.callback.time-constrained-enabled][federate.callback.time-advance-grant][federate.callback.initiate-federate-save][federate.callback.federation-saved]") {
  class ReplacementTimedSaveFederateAmbassador final
      : public NullFederateAmbassador {
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
      timeAdvanceGrantValue = time.toString();
      callbackOrder.push_back("grant");
    }
    void initiateFederateSave(
        std::wstring const& label,
        rti1516_2025::LogicalTime const& time) override {
      ++timedInitiateCount;
      timedInitiateLabels.push_back(label);
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
    std::vector<std::wstring> timedInitiateLabels;
    std::wstring timedInitiateImplementation;
    std::wstring timedInitiateValue;
    std::size_t savedCount = 0U;
    std::vector<std::string> callbackOrder;
  };

  using umbra::detail::ProcessTransportListener;
  constexpr wchar_t const* federationName =
      L"process-timed-federation-save-replacement-execution";
  constexpr wchar_t const* ownerType =
      L"process-timed-federation-save-replacement-owner-type";
  constexpr wchar_t const* receiverType =
      L"process-timed-federation-save-replacement-receiver-type";
  constexpr wchar_t const* firstSaveLabel =
      L"process-timed-federation-save-replacement-first";
  constexpr wchar_t const* replacementSaveLabel =
      L"process-timed-federation-save-replacement-final";
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
          {"process-timed-federation-save-replacement-server", 0x9740U},
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
      serveExpected(ownerSession, ownerHandler,
                    TransportServiceOperation::create_federation_execution,
                    "The save-replacement server lost Create.");
      serveExpected(ownerSession, ownerHandler,
                    TransportServiceOperation::join_federation_execution,
                    "The save-replacement server lost owner Join.");
      auto receiverConnection = listener->accept(
          nullptr,
          {"process-timed-federation-save-replacement-server", 0x9741U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiverSession(receiverConnection);
      auto receiverHandler = service.handlerFor(receiverSession);
      serveExpected(receiverSession, receiverHandler,
                    TransportServiceOperation::join_federation_execution,
                    "The save-replacement server lost receiver Join.");
      serveExpected(ownerSession, ownerHandler,
                    TransportServiceOperation::enable_time_regulation,
                    "The save-replacement server lost owner Enable Time Regulation.");
      serveExpected(receiverSession, receiverHandler,
                    TransportServiceOperation::enable_time_constrained,
                    "The save-replacement server lost receiver Enable Time Constrained.");
      serveExpected(ownerSession, ownerHandler,
                    TransportServiceOperation::request_federation_save,
                    "The save-replacement server lost first Request Federation Save.");
      serveExpected(ownerSession, ownerHandler,
                    TransportServiceOperation::request_federation_save,
                    "The save-replacement server lost replacement Request Federation Save.");
      serveExpected(ownerSession, ownerHandler,
                    TransportServiceOperation::time_advance_request,
                    "The save-replacement server lost owner TAR.");
      serveExpected(receiverSession, receiverHandler,
                    TransportServiceOperation::time_advance_request,
                    "The save-replacement server lost receiver TAR.");
      serveExpected(ownerSession, ownerHandler,
                    TransportServiceOperation::receive_interaction,
                    "The save-replacement server lost owner save polling.");
      serveExpected(ownerSession, ownerHandler,
                    TransportServiceOperation::federate_save_begun,
                    "The save-replacement server lost owner Federate Save Begun.");
      serveExpected(receiverSession, receiverHandler,
                    TransportServiceOperation::federate_save_begun,
                    "The save-replacement server lost receiver Federate Save Begun.");
      serveExpected(ownerSession, ownerHandler,
                    TransportServiceOperation::federate_save_complete,
                    "The save-replacement server lost owner Federate Save Complete.");
      serveExpected(receiverSession, receiverHandler,
                    TransportServiceOperation::federate_save_complete,
                    "The save-replacement server lost receiver Federate Save Complete.");
      serveExpected(ownerSession, ownerHandler,
                    TransportServiceOperation::receive_interaction,
                    "The save-replacement server lost owner completion polling.");
      serveExpected(receiverSession, receiverHandler,
                    TransportServiceOperation::receive_interaction,
                    "The save-replacement server lost receiver completion polling.");
      serveExpected(ownerSession, ownerHandler,
                    TransportServiceOperation::resign_federation_execution,
                    "The save-replacement server lost owner Resign.");
      serveExpected(receiverSession, receiverHandler,
                    TransportServiceOperation::resign_federation_execution,
                    "The save-replacement server lost receiver Resign.");
      service.detach(receiverSession);
      service.detach(ownerSession);
      receiverConnection->close();
      ownerConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ReplacementTimedSaveFederateAmbassador ownerFederate;
  ReplacementTimedSaveFederateAmbassador receiverFederate;
  auto ownerRti = makeRti();
  auto receiverRti = makeRti();
  auto ownerConfiguration = RtiConfiguration::createConfiguration()
                                .withConfigurationName(
                                    L"process-timed-federation-save-replacement-owner-client")
                                .withRtiAddress(
                                    L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto receiverConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"process-timed-federation-save-replacement-receiver-client")
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
                receiverFederate, HLA_EVOKED, receiverConfiguration)
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

    rti1516_2025::HLAinteger64Time firstSaveTime(5);
    rti1516_2025::HLAinteger64Time replacementSaveTime(7);
    REQUIRE_NOTHROW(ownerRti->requestFederationSave(
        firstSaveLabel, firstSaveTime));
    REQUIRE(ownerFederate.timedInitiateCount == 0U);
    REQUIRE(receiverFederate.timedInitiateCount == 0U);
    REQUIRE_NOTHROW(ownerRti->requestFederationSave(
        replacementSaveLabel, replacementSaveTime));
    REQUIRE(ownerFederate.timedInitiateCount == 0U);
    REQUIRE(receiverFederate.timedInitiateCount == 0U);

    REQUIRE_NOTHROW(ownerRti->timeAdvanceRequest(replacementSaveTime));
    REQUIRE_FALSE(ownerRti->evokeCallback(0.0));
    REQUIRE(ownerFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(ownerFederate.timeAdvanceGrantValue == replacementSaveTime.toString());
    REQUIRE(ownerFederate.timedInitiateCount == 0U);
    REQUIRE_NOTHROW(receiverRti->timeAdvanceRequest(replacementSaveTime));
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.timedInitiateCount == 1U);
    REQUIRE(receiverFederate.timedInitiateLabels ==
            std::vector<std::wstring>{replacementSaveLabel});
    REQUIRE(receiverFederate.timedInitiateImplementation == L"HLAinteger64Time");
    REQUIRE(receiverFederate.timedInitiateValue == replacementSaveTime.toString());
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(receiverFederate.timeAdvanceGrantValue == replacementSaveTime.toString());
    REQUIRE(ownerFederate.timedInitiateCount == 0U);
    REQUIRE_FALSE(ownerRti->evokeCallback(0.0));
    REQUIRE(ownerFederate.timedInitiateCount == 1U);
    REQUIRE(ownerFederate.timedInitiateLabels ==
            std::vector<std::wstring>{replacementSaveLabel});
    REQUIRE(ownerFederate.timedInitiateImplementation == L"HLAinteger64Time");
    REQUIRE(ownerFederate.timedInitiateValue == replacementSaveTime.toString());
    REQUIRE(ownerFederate.callbackOrder ==
            std::vector<std::string>{"grant", "save-initiate-timed"});
    REQUIRE(receiverFederate.callbackOrder ==
            std::vector<std::string>{"save-initiate-timed", "grant"});

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
#endif
#endif
