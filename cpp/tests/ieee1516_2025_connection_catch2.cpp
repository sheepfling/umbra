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
    "RTIambassador selects a configured tcp process endpoint through the official address field",
    "[integration][foundation][connection][transport][process-boundary][public-endpoint]") {
  using umbra::detail::ProcessTransportListener;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      auto connection = listener->accept(
          nullptr,
          {"process-endpoint-test-server", 0x9101U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"process-endpoint-test-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));

  std::exception_ptr clientError;
  std::optional<ConfigurationResult> result;
  try {
    result = rti->connect(federate, HLA_EVOKED, configuration);
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
  if (serverError) {
    std::rethrow_exception(serverError);
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE(result.has_value());
  REQUIRE(result->configurationUsed);
  REQUIRE(result->addressUsed);
  REQUIRE(result->additionalSettingsResult == SETTINGS_IGNORED);
}

TEST_CASE(
    "RTIambassador routes public Create, Join, Resign, and Destroy through a configured process endpoint",
    "[integration][foundation][federation-management][transport][time-management][2025][process-boundary][process-boundary-federation-lifecycle][public-endpoint][rti.service.enable-time-regulation][rti.service.disable-time-regulation][rti.service.query-logical-time][rti.service.destroy-federation-execution][federate.callback.time-regulation-enabled]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::FederationDefinition;
  using umbra::detail::FomModuleKind;
  using umbra::detail::PrevalidatedFomModule;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportServiceDispatcher;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      FederationDefinition definition;
      definition.fomModules.push_back(PrevalidatedFomModule{
          L"urn:umbra:test:process-public-fom",
          {},
          {},
          FomModuleKind::fom,
          L"IEEE1516-DIF-2025.xsd",
          {},
          umbra::detail::FomStandardEdition::ieee1516_2025,
          umbra::detail::FomSourceCompatibility::strict,
          {}});
      definition.logicalTimeImplementationName = L"HLAinteger64Time";
      ProcessFederationService service(registry, std::move(definition));
      auto connection = listener->accept(
          nullptr,
          {"process-public-endpoint-server", 0x9201U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      if (!umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler)) {
        throw std::runtime_error(
            "The public process endpoint server did not receive Create, Join, Destroy-while-joined, Enable/Disable Time Regulation, Query Logical Time, Resign, Destroy, and missing-Destroy.");
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessTimeFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"process-public-endpoint-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  std::optional<ConfigurationResult> connectionResult;
  std::optional<rti1516_2025::FederateHandle> joinedHandle;
  try {
    connectionResult = rti->connect(federate, HLA_EVOKED, configuration);
    rti->createFederationExecution(
        L"process-public-execution", L"server-owned-fom.xml");
    joinedHandle = rti->joinFederationExecution(
        L"process-public-type", L"process-public-execution");
    REQUIRE_THROWS_AS(
        rti->destroyFederationExecution(L"process-public-execution"),
        rti1516_2025::FederatesCurrentlyJoined);
    REQUIRE_NOTHROW(
        rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE(federate.timeRegulationEnabledCount == 0U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.timeRegulationEnabledCount == 1U);
    REQUIRE(
        federate.timeRegulationEnabledImplementation == L"HLAinteger64Time");
    rti1516_2025::HLAinteger64Time queriedTime;
    REQUIRE_NOTHROW(rti->queryLogicalTime(queriedTime));
    REQUIRE(queriedTime.getTime() == 0);
    REQUIRE_NOTHROW(rti->disableTimeRegulation());
    REQUIRE_THROWS_AS(
        rti->disableTimeRegulation(),
        rti1516_2025::TimeRegulationIsNotEnabled);
    rti->resignFederationExecution(NO_ACTION);
    REQUIRE_NOTHROW(
        rti->destroyFederationExecution(L"process-public-execution"));
    REQUIRE_THROWS_AS(
        rti->destroyFederationExecution(L"process-public-execution"),
        rti1516_2025::FederationExecutionDoesNotExist);
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
  REQUIRE(connectionResult.has_value());
  REQUIRE(connectionResult->addressUsed);
  REQUIRE(joinedHandle.has_value());
  REQUIRE(joinedHandle->isValid());
}

TEST_CASE(
    "RTIambassador projects the joined-federate MOM report path through a configured process endpoint",
    "[integration][foundation][federation-management][mom][service-report-file][service-reporting]"
    "[joined-federate-mom][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-connection-initial-record][process-service-report-file-switch-cycle]"
    "[process-service-report-file-join-lifetime]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes][rti.service.evoke-callback]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.send-interaction][rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.request-attribute-value-update][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]") {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    struct Discovery final {
      rti1516_2025::ObjectInstanceHandle objectInstance;
      rti1516_2025::ObjectClassHandle objectClass;
      std::wstring objectInstanceName;
      rti1516_2025::FederateHandle producingFederate;
    };
    struct Reflection final {
      rti1516_2025::ObjectInstanceHandle objectInstance;
      rti1516_2025::AttributeHandleValueMap attributeValues;
      rti1516_2025::FederateHandle producingFederate;
    };
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const& objectClass,
        std::wstring const& objectInstanceName,
        rti1516_2025::FederateHandle const& producingFederate) override {
      discoveries.push_back({objectInstance, objectClass, objectInstanceName, producingFederate});
    }
    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const&,
        rti1516_2025::TransportationTypeHandle const&,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const*) override {
      reflections.push_back({objectInstance, attributeValues, producingFederate});
      reflectionCount.fetch_add(1U, std::memory_order_release);
      reflectionChanged.notify_all();
      if (discoveries.size() >= 2U && reflections.size() >= 2U) {
        initialMomValuesReady.store(true, std::memory_order_release);
      }
    }
    bool waitForReflectionCount(std::size_t expectedCount) {
      std::unique_lock lock(reflectionMutex);
      return reflectionChanged.wait_for(
          lock,
          std::chrono::milliseconds(100),
          [&] {
            return reflectionCount.load(std::memory_order_acquire) >= expectedCount;
          });
    }
    std::vector<Discovery> discoveries;
    std::vector<Reflection> reflections;
    std::mutex reflectionMutex;
    std::condition_variable reflectionChanged;
    std::atomic_size_t reflectionCount{0U};
    std::atomic_bool initialMomValuesReady{false};
  } observerFederate;

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
      L"public-process-joined-federate-mom-report-execution";
  constexpr wchar_t const* subjectName =
      L"public-process-joined-federate-mom-report-subject";
  constexpr wchar_t const* observerName =
      L"public-process-joined-federate-mom-report-observer";

  std::exception_ptr serverError;
  auto narrowExceptionText = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      result.push_back(character >= 0 && character <= 0x7F
                           ? static_cast<char>(character)
                           : '?');
    }
    return result;
  };
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
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
      auto subjectConnection = listener->accept(
          nullptr,
          {"public-process-joined-federate-mom-report-server", 0x9D01U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession subjectSession(subjectConnection);
      auto subjectHandler = service.handlerFor(subjectSession);
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::create_federation_execution,
                    "The process MOM server lost Create.");
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::join_federation_execution,
                    "The process MOM server lost subject Join.");
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::get_send_service_reports_to_file_switch,
                    "The process MOM server lost the initial subject file-switch query.");
      auto observerConnection = listener->accept(
          nullptr,
          {"public-process-joined-federate-mom-report-server", 0x9D02U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession observerSession(observerConnection);
      auto observerHandler = service.handlerFor(observerSession);
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::join_federation_execution,
                    "The process MOM server lost observer Join.");
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::get_object_class_handle,
                    "The process MOM server lost the MOM class lookup.");
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::get_attribute_handle,
                    "The process MOM server lost the MOM report-file lookup.");
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::get_attribute_handle,
                    "The process MOM server lost the MOM name lookup.");
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::subscribe_object_class_attributes,
                    "The process MOM server lost the MOM subscription.");
      while (!observerFederate.initialMomValuesReady.load(std::memory_order_acquire)) {
        serveExpected(observerSession, observerHandler,
                      TransportServiceOperation::receive_interaction,
                      "The process MOM server lost an initial callback poll.");
      }
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::get_interaction_class_handle,
                    "The process MOM server lost the HLAsetSwitches lookup.");
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::get_parameter_handle,
                    "The process MOM server lost the file-switch parameter lookup.");
      auto serveObserverReflectionsUntil = [&](std::size_t expectedCount) {
        std::size_t pollCount = 0U;
        while (observerFederate.reflectionCount.load(std::memory_order_acquire) <
                   expectedCount &&
               pollCount != 32U) {
          serveExpected(observerSession, observerHandler,
                        TransportServiceOperation::receive_interaction,
                        "The process MOM server lost a requested report-path reflection poll.");
          ++pollCount;
          if (observerFederate.reflectionCount.load(std::memory_order_acquire) <
              expectedCount) {
            static_cast<void>(observerFederate.waitForReflectionCount(expectedCount));
          }
        }
        if (observerFederate.reflectionCount.load(std::memory_order_acquire) <
            expectedCount) {
          throw std::runtime_error(
              "The process MOM server did not observe the expected report-path reflection count.");
        }
        serveExpected(observerSession, observerHandler,
                      TransportServiceOperation::receive_interaction,
                      "The process MOM server lost the final empty callback poll.");
      };
      for (std::size_t expectedReflectionCount = 3U;
           expectedReflectionCount <= 5U;
           ++expectedReflectionCount) {
        serveExpected(subjectSession, subjectHandler,
                      TransportServiceOperation::send_interaction,
                      "The process MOM server lost an HLAsetSwitches file-reporting adjustment.");
        serveExpected(subjectSession, subjectHandler,
                      TransportServiceOperation::report_successful_void_service_invocation,
                      "The process MOM server lost the successful HLAsetSwitches invocation report.");
        serveExpected(subjectSession, subjectHandler,
                      TransportServiceOperation::get_send_service_reports_to_file_switch,
                      "The process MOM server lost a subject file-switch readback.");
        serveExpected(observerSession, observerHandler,
                      TransportServiceOperation::request_attribute_value_update,
                      "The process MOM server lost a report-path value request.");
        serveObserverReflectionsUntil(expectedReflectionCount);
      }
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::resign_federation_execution,
                    "The process MOM server lost the first subject Resign.");
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::join_federation_execution,
                    "The process MOM server lost the subject rejoin.");
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::get_send_service_reports_to_file_switch,
                    "The process MOM server lost the rejoined subject file-switch query.");
      std::exception_ptr observerLifecycleServerError;
      std::thread observerLifecycleServer([&] {
        try {
          while (umbra::test::servePrimaryProcessRequest(
              observerSession, observerHandler)) {
          }
        } catch (...) {
          observerLifecycleServerError = std::current_exception();
        }
      });
      try {
        serveExpected(subjectSession, subjectHandler,
                      TransportServiceOperation::resign_federation_execution,
                      "The process MOM server lost the final subject Resign.");
      } catch (...) {
        observerConnection->close();
        if (observerLifecycleServer.joinable()) {
          observerLifecycleServer.join();
        }
        throw;
      }
      if (observerLifecycleServer.joinable()) {
        observerLifecycleServer.join();
      }
      if (observerLifecycleServerError) {
        std::rethrow_exception(observerLifecycleServerError);
      }
      service.detach(subjectSession);
      service.detach(observerSession);
      subjectConnection->close();
      observerConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador subjectFederate;
  auto subject = makeRti();
  auto observer = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"public-process-joined-federate-mom-report-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port))
                           .withAdditionalSettings(L"ignored-by-initial-slice");
  std::exception_ptr clientError;
  bool subjectJoined = false;
  bool observerJoined = false;
  try {
    rti1516_2025::HLAnoCredentials credentials;
    REQUIRE(subject->connect(
                subjectFederate, HLA_IMMEDIATE, configuration, credentials)
                .addressUsed);
    subject->createFederationExecution(federationName, L"server-owned-fom.xml");
    REQUIRE(subject->joinFederationExecution(
                subjectName,
                L"public-process-joined-federate-mom-report-type",
                federationName)
                .isValid());
    subjectJoined = true;
    REQUIRE_FALSE(subject->getSendServiceReportsToFileSwitch());
    REQUIRE(observer->connect(observerFederate, HLA_EVOKED, configuration).addressUsed);
    REQUIRE(observer->joinFederationExecution(
                observerName,
                L"public-process-joined-federate-mom-report-type",
                federationName)
                .isValid());
    observerJoined = true;

    auto const momClass = observer->getObjectClassHandle(
        L"HLAobjectRoot.HLAmanager.HLAfederate");
    auto const reportFileAttribute =
        observer->getAttributeHandle(momClass, L"HLAreportServiceFile");
    auto const federateNameAttribute =
        observer->getAttributeHandle(momClass, L"HLAfederateName");
    REQUIRE(momClass.isValid());
    REQUIRE(reportFileAttribute.isValid());
    REQUIRE(federateNameAttribute.isValid());
    REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
        momClass,
        rti1516_2025::AttributeHandleSet{reportFileAttribute, federateNameAttribute}));
    for (std::size_t pass = 0U; pass != 16U &&
         !observerFederate.initialMomValuesReady.load(std::memory_order_acquire);
         ++pass) {
      static_cast<void>(observer->evokeCallback(0.0));
    }
    // The server's receive-order loop is intentionally request/response
    // driven.  Send one final empty poll after the callback has become ready
    // so the server cannot be left blocked in serveOne between the last
    // reflection and the teardown requests.
    static_cast<void>(observer->evokeCallback(0.0));
    REQUIRE(observerFederate.initialMomValuesReady.load(std::memory_order_acquire));
    REQUIRE(observerFederate.discoveries.size() == 2U);
    REQUIRE(observerFederate.reflections.size() == 2U);

    std::optional<RecordingFederateAmbassador::Reflection> subjectReflection;
    for (auto const& reflection : observerFederate.reflections) {
      auto const value = reflection.attributeValues.find(federateNameAttribute);
      if (value == reflection.attributeValues.end()) {
        continue;
      }
      rti1516_2025::HLAunicodeString reflectedName;
      try {
        reflectedName.decode(value->second);
      } catch (...) {
        continue;
      }
      if (reflectedName.get() == subjectName) {
        subjectReflection = reflection;
        break;
      }
    }
    REQUIRE(subjectReflection.has_value());
    REQUIRE(subjectReflection->attributeValues.size() == 2U);
    REQUIRE_FALSE(subjectReflection->producingFederate.isValid());
    auto const discovery = std::find_if(
        observerFederate.discoveries.begin(),
        observerFederate.discoveries.end(),
        [&](RecordingFederateAmbassador::Discovery const& value) {
          return value.objectInstance == subjectReflection->objectInstance;
        });
    REQUIRE(discovery != observerFederate.discoveries.end());
    REQUIRE(discovery->objectClass == momClass);
    REQUIRE_FALSE(discovery->producingFederate.isValid());

    rti1516_2025::HLAunicodeString advertisedPath;
    REQUIRE_NOTHROW(advertisedPath.decode(
        subjectReflection->attributeValues.at(reportFileAttribute)));
    auto const reportPath = std::filesystem::path(advertisedPath.get());
    REQUIRE(reportPath.is_absolute());
    REQUIRE(reportPath.lexically_normal() == reportPath);
    REQUIRE(reportPath.parent_path() ==
            std::filesystem::absolute(reportDirectory).lexically_normal());
    REQUIRE(std::filesystem::exists(reportPath));
    std::ifstream reportStream(reportPath, std::ios::binary);
    REQUIRE(reportStream.good());
    std::string reportText{
        std::istreambuf_iterator<char>(reportStream),
        std::istreambuf_iterator<char>{}};
    reportStream.close();
    REQUIRE(reportText.find("\"CallbackModel\":\"HLA_IMMEDIATE\"") !=
            std::string::npos);
    REQUIRE(reportText.find(
                "\"ConfigurationName\":\"public-process-joined-federate-mom-report-client\"") !=
            std::string::npos);
    REQUIRE(reportText.find(
                "\"RTIaddress\":\"tcp://127.0.0.1:" + std::to_string(port) + "\"") !=
            std::string::npos);
    REQUIRE(reportText.find(
                "\"AdditionalSettings\":\"ignored-by-initial-slice\"") !=
            std::string::npos);
    REQUIRE(reportText.find(
                "\"Credentials\":{\"Type\":\"HLAnoCredentials\",\"Data\":\"\"}") !=
            std::string::npos);

    auto const setSwitches = subject->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
    auto const sendServiceReportsToFile = subject->getParameterHandle(
        setSwitches, L"HLAsendServiceReportsToFile");
    REQUIRE(setSwitches.isValid());
    REQUIRE(sendServiceReportsToFile.isValid());
    auto encodeSwitch = [](bool enabled) {
      return rti1516_2025::HLAinteger32BE(enabled ? 1 : 0).encode();
    };
    auto requestAndCheckStableReportPath = [&](
        rti1516_2025::ObjectInstanceHandle expectedObjectInstance,
        std::wstring const& expectedPath) {
      auto const reflectionsBefore = observerFederate.reflectionCount.load(
          std::memory_order_acquire);
      try {
        observer->requestAttributeValueUpdate(
            expectedObjectInstance,
            rti1516_2025::AttributeHandleSet{reportFileAttribute},
            rti1516_2025::VariableLengthData{});
      } catch (rti1516_2025::Exception const& error) {
        throw std::runtime_error(
            narrowExceptionText(error.name()) + ": " +
            narrowExceptionText(error.what()));
      }
      for (std::size_t pass = 0U;
           pass != 64U &&
           observerFederate.reflectionCount.load(std::memory_order_acquire) ==
               reflectionsBefore;
           ++pass) {
        static_cast<void>(observer->evokeCallback(0.0));
      }
      // Match one final empty process poll if the server has not yet observed
      // the callback-count transition from the last requested-value response.
      static_cast<void>(observer->evokeCallback(0.0));
      REQUIRE(observerFederate.reflectionCount.load(std::memory_order_acquire) ==
              reflectionsBefore + 1U);
      auto const& requestedReflection = observerFederate.reflections.back();
      REQUIRE(requestedReflection.objectInstance == expectedObjectInstance);
      REQUIRE(requestedReflection.attributeValues.size() == 1U);
      rti1516_2025::HLAunicodeString requestedPath;
      REQUIRE_NOTHROW(requestedPath.decode(
          requestedReflection.attributeValues.at(reportFileAttribute)));
      REQUIRE(requestedPath.get() == expectedPath);
    };

    rti1516_2025::ParameterHandleValueMap const enabledFileSwitch{
        {sendServiceReportsToFile, encodeSwitch(true)}};
    REQUIRE_NOTHROW(subject->sendInteraction(
        setSwitches, enabledFileSwitch, rti1516_2025::VariableLengthData{}));
    REQUIRE(subject->getSendServiceReportsToFileSwitch());
    requestAndCheckStableReportPath(
        subjectReflection->objectInstance, advertisedPath.get());

    rti1516_2025::ParameterHandleValueMap const disabledFileSwitch{
        {sendServiceReportsToFile, encodeSwitch(false)}};
    REQUIRE_NOTHROW(subject->sendInteraction(
        setSwitches, disabledFileSwitch, rti1516_2025::VariableLengthData{}));
    REQUIRE_FALSE(subject->getSendServiceReportsToFileSwitch());
    requestAndCheckStableReportPath(
        subjectReflection->objectInstance, advertisedPath.get());

    REQUIRE_NOTHROW(subject->sendInteraction(
        setSwitches, enabledFileSwitch, rti1516_2025::VariableLengthData{}));
    REQUIRE(subject->getSendServiceReportsToFileSwitch());
    requestAndCheckStableReportPath(
        subjectReflection->objectInstance, advertisedPath.get());

    auto readFile = [](std::filesystem::path const& path) {
      std::ifstream stream(path, std::ios::binary);
      return std::string{
          std::istreambuf_iterator<char>(stream),
          std::istreambuf_iterator<char>{}};
    };
    subject->resignFederationExecution(NO_ACTION);
    subjectJoined = false;
    auto const originalFileContents = readFile(reportPath);
    REQUIRE_FALSE(originalFileContents.empty());
    auto const rejoinedSubject = subject->joinFederationExecution(
        subjectName,
        L"public-process-joined-federate-mom-report-type",
        federationName);
    if (!rejoinedSubject.isValid()) {
      throw std::runtime_error(
          "The process rejoin returned an invalid public federate handle.");
    }
    subjectJoined = true;
    static_cast<void>(subject->getSendServiceReportsToFileSwitch());

    for (std::size_t pass = 0U;
         pass != 64U &&
         observerFederate.reflectionCount.load(std::memory_order_acquire) < 6U;
         ++pass) {
      static_cast<void>(observer->evokeCallback(0.0));
    }
    static_cast<void>(observer->evokeCallback(0.0));
    REQUIRE(observerFederate.reflectionCount.load(std::memory_order_acquire) == 6U);
    REQUIRE(observerFederate.discoveries.size() == 3U);
    REQUIRE(observerFederate.reflections.size() == 6U);
    auto const& rejoinedReflection = observerFederate.reflections.back();
    REQUIRE(rejoinedReflection.objectInstance != subjectReflection->objectInstance);
    REQUIRE(rejoinedReflection.attributeValues.size() == 2U);
    rti1516_2025::HLAunicodeString rejoinedFederateName;
    REQUIRE_NOTHROW(rejoinedFederateName.decode(
        rejoinedReflection.attributeValues.at(federateNameAttribute)));
    REQUIRE(rejoinedFederateName.get() == subjectName);
    rti1516_2025::HLAunicodeString rejoinedAdvertisedPath;
    REQUIRE_NOTHROW(rejoinedAdvertisedPath.decode(
        rejoinedReflection.attributeValues.at(reportFileAttribute)));
    auto const rejoinedReportPath =
        std::filesystem::path(rejoinedAdvertisedPath.get());
    REQUIRE(rejoinedReportPath.is_absolute());
    REQUIRE(rejoinedReportPath.lexically_normal() == rejoinedReportPath);
    REQUIRE(rejoinedReportPath.parent_path() == reportPath.parent_path());
    REQUIRE(rejoinedReportPath != reportPath);
    REQUIRE(std::filesystem::exists(rejoinedReportPath));
    auto const rejoinedFileContents = readFile(rejoinedReportPath);
    REQUIRE_FALSE(rejoinedFileContents.empty());
    REQUIRE(rejoinedFileContents.find(
                "\"ConfigurationName\":\"public-process-joined-federate-mom-report-client\"") !=
            std::string::npos);
    REQUIRE(readFile(reportPath) == originalFileContents);
    requestAndCheckStableReportPath(
        rejoinedReflection.objectInstance, rejoinedAdvertisedPath.get());

    subject->resignFederationExecution(NO_ACTION);
    subjectJoined = false;
    observer->resignFederationExecution(NO_ACTION);
    observerJoined = false;
    subject->disconnect();
    observer->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    try {
      std::rethrow_exception(clientError);
    } catch (rti1516_2025::Exception const& error) {
      clientError = std::make_exception_ptr(std::runtime_error(
          narrowExceptionText(error.name()) + ": " +
          narrowExceptionText(error.what())));
    } catch (...) {
    }
    if (subjectJoined) {
      try {
        subject->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (observerJoined) {
      try {
        observer->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      subject->disconnect();
    } catch (...) {
    }
    try {
      observer->disconnect();
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
  REQUIRE_FALSE(subjectJoined);
  REQUIRE_FALSE(observerJoined);
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
}

TEST_CASE(
    "RTIambassador delivers resigned HLAfederate MOM removal before rejoined-object discovery",
    "[integration][internal][foundation][federation-management][mom]"
    "[transport][process-boundary][public-endpoint][2025][process-mom-resign-removal-order]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.resign-federation-execution]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes][rti.service.evoke-callback]"
    "[rti.service.disconnect][federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values][federate.callback.remove-object-instance]") {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    enum class CallbackKind { discovery, reflection, removal };
    struct CallbackEvent final {
      CallbackKind kind;
      rti1516_2025::ObjectInstanceHandle objectInstance;
    };
    struct Reflection final {
      rti1516_2025::ObjectInstanceHandle objectInstance;
      rti1516_2025::AttributeHandleValueMap attributeValues;
    };

    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const&,
        std::wstring const&,
        rti1516_2025::FederateHandle const&) override {
      callbackEvents.push_back({CallbackKind::discovery, objectInstance});
    }

    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const&,
        rti1516_2025::TransportationTypeHandle const&,
        rti1516_2025::FederateHandle const&,
        rti1516_2025::RegionHandleSet const*) override {
      callbackEvents.push_back({CallbackKind::reflection, objectInstance});
      reflections.push_back({objectInstance, attributeValues});
    }

    void removeObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::VariableLengthData const&,
        rti1516_2025::FederateHandle const&) override {
      callbackEvents.push_back({CallbackKind::removal, objectInstance});
    }

    std::vector<CallbackEvent> callbackEvents;
    std::vector<Reflection> reflections;
  } observerFederate;

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
      L"public-process-mom-resign-removal-order-execution";
  constexpr wchar_t const* subjectName = L"public-process-mom-removal-subject";
  constexpr wchar_t const* observerName = L"public-process-mom-removal-observer";

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
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

      auto subjectConnection = listener->accept(
          nullptr,
          {"public-process-mom-removal-server", 0x9D11U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession subjectSession(subjectConnection);
      auto subjectHandler = service.handlerFor(subjectSession);
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::create_federation_execution,
                    "The process MOM lifecycle server lost Create.");
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::join_federation_execution,
                    "The process MOM lifecycle server lost the initial subject Join.");

      auto observerConnection = listener->accept(
          nullptr,
          {"public-process-mom-removal-server", 0x9D12U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession observerSession(observerConnection);
      auto observerHandler = service.handlerFor(observerSession);
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::join_federation_execution,
                    "The process MOM lifecycle server lost observer Join.");
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::get_object_class_handle,
                    "The process MOM lifecycle server lost the MOM class lookup.");
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::get_attribute_handle,
                    "The process MOM lifecycle server lost the federate-name lookup.");
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::subscribe_object_class_attributes,
                    "The process MOM lifecycle server lost the MOM subscription.");

      std::exception_ptr observerServerError;
      std::thread observerServer([&] {
        try {
          while (umbra::test::servePrimaryProcessRequest(
              observerSession, observerHandler)) {
          }
        } catch (...) {
          observerServerError = std::current_exception();
        }
      });
      try {
        serveExpected(subjectSession, subjectHandler,
                      TransportServiceOperation::resign_federation_execution,
                      "The process MOM lifecycle server lost the subject Resign.");
        serveExpected(subjectSession, subjectHandler,
                      TransportServiceOperation::join_federation_execution,
                      "The process MOM lifecycle server lost the subject rejoin.");
        serveExpected(subjectSession, subjectHandler,
                      TransportServiceOperation::resign_federation_execution,
                      "The process MOM lifecycle server lost the final subject Resign.");
      } catch (...) {
        observerConnection->close();
        if (observerServer.joinable()) {
          observerServer.join();
        }
        throw;
      }
      if (observerServer.joinable()) {
        observerServer.join();
      }
      if (observerServerError) {
        std::rethrow_exception(observerServerError);
      }
      service.detach(subjectSession);
      service.detach(observerSession);
      subjectConnection->close();
      observerConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  NullFederateAmbassador subjectFederate;
  auto subject = makeRti();
  auto observer = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"public-process-mom-removal-order-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  constexpr wchar_t const* federateType = L"public-process-mom-removal-order-type";
  std::exception_ptr clientError;
  bool subjectJoined = false;
  bool observerJoined = false;
  try {
    rti1516_2025::HLAnoCredentials credentials;
    REQUIRE(subject->connect(
                subjectFederate, HLA_IMMEDIATE, configuration, credentials)
                .addressUsed);
    subject->createFederationExecution(federationName, L"server-owned-fom.xml");
    REQUIRE(subject->joinFederationExecution(
                subjectName, federateType, federationName)
                .isValid());
    subjectJoined = true;

    REQUIRE(observer->connect(observerFederate, HLA_EVOKED, configuration).addressUsed);
    REQUIRE(observer->joinFederationExecution(
                observerName, federateType, federationName)
                .isValid());
    observerJoined = true;
    auto const momClass = observer->getObjectClassHandle(
        L"HLAobjectRoot.HLAmanager.HLAfederate");
    auto const federateNameAttribute =
        observer->getAttributeHandle(momClass, L"HLAfederateName");
    REQUIRE(momClass.isValid());
    REQUIRE(federateNameAttribute.isValid());
    REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
        momClass, rti1516_2025::AttributeHandleSet{federateNameAttribute}));

    auto subjectReflectionForName = [&](
                                           std::optional<rti1516_2025::ObjectInstanceHandle>
                                               excludedObject = std::nullopt) {
      return std::find_if(
          observerFederate.reflections.begin(), observerFederate.reflections.end(),
          [&](RecordingFederateAmbassador::Reflection const& reflection) {
            if (excludedObject && reflection.objectInstance == *excludedObject) {
              return false;
            }
            auto const value = reflection.attributeValues.find(federateNameAttribute);
            if (value == reflection.attributeValues.end()) {
              return false;
            }
            rti1516_2025::HLAunicodeString name;
            try {
              name.decode(value->second);
            } catch (...) {
              return false;
            }
            return name.get() == subjectName;
          });
    };
    for (std::size_t pass = 0U;
         pass != 64U && observerFederate.reflections.size() < 2U;
         ++pass) {
      static_cast<void>(observer->evokeCallback(0.0));
    }
    REQUIRE(observerFederate.reflections.size() >= 2U);
    auto const initialSubjectReflection = subjectReflectionForName();
    REQUIRE(initialSubjectReflection != observerFederate.reflections.end());
    auto const resignedObject = initialSubjectReflection->objectInstance;

    subject->resignFederationExecution(NO_ACTION);
    subjectJoined = false;
    REQUIRE(subject->joinFederationExecution(
                subjectName, federateType, federationName)
                .isValid());
    subjectJoined = true;

    for (std::size_t pass = 0U;
         pass != 128U &&
         std::none_of(
             observerFederate.reflections.begin(), observerFederate.reflections.end(),
             [&](RecordingFederateAmbassador::Reflection const& reflection) {
               if (reflection.objectInstance == resignedObject) {
                 return false;
               }
               auto const value = reflection.attributeValues.find(federateNameAttribute);
               if (value == reflection.attributeValues.end()) {
                 return false;
               }
               rti1516_2025::HLAunicodeString name;
               try {
                 name.decode(value->second);
               } catch (...) {
                 return false;
               }
               return name.get() == subjectName;
             });
         ++pass) {
      static_cast<void>(observer->evokeCallback(0.0));
    }
    auto const rejoinedSubjectReflection = subjectReflectionForName(resignedObject);
    REQUIRE(rejoinedSubjectReflection != observerFederate.reflections.end());

    auto const removal = std::find_if(
        observerFederate.callbackEvents.begin(), observerFederate.callbackEvents.end(),
        [&](RecordingFederateAmbassador::CallbackEvent const& event) {
          return event.kind == RecordingFederateAmbassador::CallbackKind::removal &&
                 event.objectInstance == resignedObject;
        });
    auto const rejoinedDiscovery = std::find_if(
        observerFederate.callbackEvents.begin(), observerFederate.callbackEvents.end(),
        [&](RecordingFederateAmbassador::CallbackEvent const& event) {
          return event.kind == RecordingFederateAmbassador::CallbackKind::discovery &&
                 event.objectInstance == rejoinedSubjectReflection->objectInstance;
        });
    auto const rejoinedReflection = std::find_if(
        observerFederate.callbackEvents.begin(), observerFederate.callbackEvents.end(),
        [&](RecordingFederateAmbassador::CallbackEvent const& event) {
          return event.kind == RecordingFederateAmbassador::CallbackKind::reflection &&
                 event.objectInstance == rejoinedSubjectReflection->objectInstance;
        });
    REQUIRE(removal != observerFederate.callbackEvents.end());
    REQUIRE(rejoinedDiscovery != observerFederate.callbackEvents.end());
    REQUIRE(rejoinedReflection != observerFederate.callbackEvents.end());
    REQUIRE(removal < rejoinedDiscovery);
    REQUIRE(rejoinedDiscovery < rejoinedReflection);

    subject->resignFederationExecution(NO_ACTION);
    subjectJoined = false;
    observer->resignFederationExecution(NO_ACTION);
    observerJoined = false;
    subject->disconnect();
    observer->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (subjectJoined) {
      try {
        subject->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (observerJoined) {
      try {
        observer->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      subject->disconnect();
    } catch (...) {
    }
    try {
      observer->disconnect();
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
  REQUIRE_FALSE(subjectJoined);
  REQUIRE_FALSE(observerJoined);
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
}

TEST_CASE("IEEE 1516.1-2025 connection support types have usable value semantics", "[baseline][support-types][unit][foundation]") {
  RtiConfiguration configuration = RtiConfiguration::createConfiguration()
                                     .withConfigurationName(L"embedded")
                                     .withRtiAddress(L"in-process")
                                     .withAdditionalSettings(L"callbacks=evoked");
  REQUIRE(configuration.configurationName() == L"embedded");
  REQUIRE(configuration.rtiAddress() == L"in-process");
  REQUIRE(configuration.additionalSettings() == L"callbacks=evoked");

  ConfigurationResult defaultResult;
  requireIgnoredConfiguration(defaultResult);

  ConfigurationResult configuredResult(
      true,
      false,
      SETTINGS_APPLIED,
      L"configuration and additional settings were applied");
  REQUIRE(configuredResult.configurationUsed);
  REQUIRE_FALSE(configuredResult.addressUsed);
  REQUIRE(configuredResult.additionalSettingsResult == SETTINGS_APPLIED);
  REQUIRE(configuredResult.message == L"configuration and additional settings were applied");

  std::array<unsigned char, 3> source{0x01, 0x02, 0x03};
  VariableLengthData value(source.data(), source.size());
  source[0] = 0xFF;
  REQUIRE(value.size() == 3);
  REQUIRE(std::memcmp(value.data(), "\x01\x02\x03", value.size()) == 0);

  VariableLengthData copied(value);
  REQUIRE(copied.size() == value.size());
  REQUIRE(std::memcmp(copied.data(), value.data(), value.size()) == 0);
}

TEST_CASE("RTIambassador Connect exposes all four official C++ overloads", "[integration][connection][federation-management]") {
  TestFederateAmbassador federate;
  HLAnoCredentials credentials;
  RtiConfiguration configuration = RtiConfiguration::createConfiguration()
                                     .withConfigurationName(L"embedded")
                                     .withRtiAddress(L"in-process")
                                     .withAdditionalSettings(L"ignored-by-initial-slice");

  SECTION("base overload") {
    auto rti = makeRti();
    requireIgnoredConfiguration(rti->connect(federate, HLA_IMMEDIATE));
    REQUIRE_THROWS_AS(rti->connect(federate, HLA_IMMEDIATE), rti1516_2025::AlreadyConnected);
    REQUIRE_NOTHROW(rti->disconnect());
  }

  SECTION("configuration overload") {
    auto rti = makeRti();
    requireIgnoredConfiguration(rti->connect(federate, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(rti->disconnect());
  }

  SECTION("credentials overload") {
    auto rti = makeRti();
    requireIgnoredConfiguration(rti->connect(federate, HLA_EVOKED, credentials));
    REQUIRE_NOTHROW(rti->disconnect());
  }

  SECTION("configuration and credentials overload") {
    auto rti = makeRti();
    requireIgnoredConfiguration(rti->connect(federate, HLA_EVOKED, configuration, credentials));
    REQUIRE_NOTHROW(rti->disconnect());
  }
}

TEST_CASE(
    "Embedded Connect rejects supplied credentials while authorization is disabled",
    "[integration][connection][authorization][credentials][federation-management]"
    "[connect-credentials-authorization-disabled]") {
  TestFederateAmbassador federate;
  HLAplainTextPassword password(L"test-password");
  RtiConfiguration configuration = RtiConfiguration::createConfiguration()
                                     .withConfigurationName(L"embedded")
                                     .withRtiAddress(L"in-process");

  SECTION("credentials overload") {
    auto rti = makeRti();
    REQUIRE_THROWS_AS(rti->connect(federate, HLA_EVOKED, password), Unauthorized);
    REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
    REQUIRE_NOTHROW(rti->disconnect());
  }

  SECTION("configuration and credentials overload") {
    auto rti = makeRti();
    REQUIRE_THROWS_AS(
        rti->connect(federate, HLA_EVOKED, configuration, password),
        Unauthorized);
    REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
    REQUIRE_NOTHROW(rti->disconnect());
  }
}

TEST_CASE(
    "Embedded Connect accepts only its filesystem service-report directory setting",
    "[integration][connection][mom][service-reporting]") {
  TestFederateAmbassador federate;
  auto const directory = temporaryServiceReportDirectory();
  RtiConfiguration configuration = RtiConfiguration::createConfiguration()
                                     .withAdditionalSettings(
                                         L"serviceReportDirectory=" + directory.wstring());
  auto rti = makeRti();

  auto const result = rti->connect(federate, HLA_EVOKED, configuration);
  REQUIRE(result.configurationUsed);
  REQUIRE_FALSE(result.addressUsed);
  REQUIRE(result.additionalSettingsResult == SETTINGS_APPLIED);
  REQUIRE(std::filesystem::is_directory(directory));
  REQUIRE_NOTHROW(rti->disconnect());

  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Umbra embedded profile configuration exposes a typed service-report directory",
    "[internal][integration][connection][mom][service-report-store][service-reporting][configuration][typed-service-report-directory-configuration]") {
  auto const directory = temporaryServiceReportDirectory();
  auto configuration = umbra::embedded::makeEmbeddedRtiConfiguration(
      umbra::embedded::ServiceReportConfiguration{directory});
  configuration.withConfigurationName(L"typed-service-report")
      .withRtiAddress(L"in-process");

  REQUIRE(configuration.additionalSettings() ==
          L"serviceReportDirectory=" + directory.wstring());
  REQUIRE(configuration.configurationName() == L"typed-service-report");
  REQUIRE(configuration.rtiAddress() == L"in-process");

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto const result = rti->connect(federate, HLA_EVOKED, configuration);
  REQUIRE(result.configurationUsed);
  REQUIRE_FALSE(result.addressUsed);
  REQUIRE(result.additionalSettingsResult == SETTINGS_APPLIED);
  REQUIRE(std::filesystem::is_directory(directory));
  REQUIRE_NOTHROW(rti->disconnect());

  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Embedded Connect fails deterministically for an unusable service-report directory",
    "[integration][connection][mom][service-reporting]") {
  TestFederateAmbassador federate;
  auto const parent = temporaryServiceReportDirectory();
  std::filesystem::create_directories(parent);
  auto const regularFile = parent / "not-a-directory";
  std::ofstream output(regularFile, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output.close();

  RtiConfiguration configuration = RtiConfiguration::createConfiguration()
                                     .withAdditionalSettings(
                                         L"serviceReportDirectory=" + regularFile.wstring());
  auto rti = makeRti();
  REQUIRE_THROWS_AS(rti->connect(federate, HLA_EVOKED, configuration), RTIinternalError);
  // A rejected configuration is not a partial connection and never falls
  // back to the in-memory test store.
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());

  std::error_code ignored;
  std::filesystem::remove_all(parent, ignored);
}

TEST_CASE(
    "Embedded Connect reports AlreadyConnected before validating service-report configuration",
    "[integration][connection][mom][service-report-store][service-reporting]") {
  TestFederateAmbassador firstFederate;
  TestFederateAmbassador secondFederate;
  auto rti = makeRti();
  REQUIRE_NOTHROW(rti->connect(firstFederate, HLA_EVOKED));

  auto const parent = temporaryServiceReportDirectory();
  std::filesystem::create_directories(parent);
  auto const regularFile = parent / "not-a-directory";
  std::ofstream output(regularFile, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output << "not a directory";
  REQUIRE(output.good());
  output.close();

  auto const invalidConfiguration = RtiConfiguration::createConfiguration()
                                        .withAdditionalSettings(
                                            L"serviceReportDirectory=" + regularFile.wstring());
  // AlreadyConnected is a lifecycle precondition. It must be resolved before
  // the second configuration can validate or mutate the report-store path.
  REQUIRE_THROWS_AS(
      rti->connect(secondFederate, HLA_EVOKED, invalidConfiguration),
      rti1516_2025::AlreadyConnected);
  REQUIRE_NOTHROW(rti->disconnect());

  std::error_code ignored;
  std::filesystem::remove_all(parent, ignored);
}

TEST_CASE("RTIambassador Connect rejects unsupported callback models without connecting", "[integration][connection][federation-management]") {
  TestFederateAmbassador federate;
  auto rti = makeRti();

  REQUIRE_THROWS_AS(
      rti->connect(federate, static_cast<CallbackModel>(-1)),
      rti1516_2025::UnsupportedCallbackModel);
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE("RTIambassador Disconnect rejects an absent connection", "[integration][connection][federation-management]") {
  auto rti = makeRti();
  REQUIRE_THROWS_AS(rti->disconnect(), rti1516_2025::NotConnected);
}

TEST_CASE("RTIambassador callback controls honor both models with an empty embedded queue", "[integration][callbacks][federation-management]") {
  TestFederateAmbassador federate;

  SECTION("immediate callbacks make Evoke services a no-op") {
    auto rti = makeRti();
    REQUIRE_NOTHROW(rti->connect(federate, HLA_IMMEDIATE));
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE_NOTHROW(rti->disableCallbacks());
    REQUIRE_NOTHROW(rti->enableCallbacks());
    REQUIRE_NOTHROW(rti->disconnect());
  }

  SECTION("evoked callbacks report no pending work after controls are toggled") {
    auto rti = makeRti();
    REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
    REQUIRE_NOTHROW(rti->disableCallbacks());
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE_NOTHROW(rti->enableCallbacks());
    REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE_NOTHROW(rti->disconnect());
  }
}

TEST_CASE(
    "RTIambassador disconnect terminates an unjoined connection",
    "[integration][compliance][rti.service.disconnect][federation-management]") {
  TestFederateAmbassador federate;
  auto rti = makeRti();

  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());
}

#endif
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassador delivers regional Attribute Relevance Advisory transitions through a configured process endpoint",
    "[integration][foundation][data-distribution-management][object-management][callbacks][transport][process-boundary][public-endpoint][regional-attribute-relevance][2025][rti.service.get-attribute-relevance-advisory-switch][rti.service.set-attribute-relevance-advisory-switch][rti.service.create-region][rti.service.commit-region-modifications][rti.service.subscribe-object-class-attributes-with-regions][rti.service.register-object-instance-with-regions][rti.service.associate-regions-for-updates][rti.service.unassociate-regions-for-updates][federate.callback.turn-updates-on-for-object-instance][federate.callback.turn-updates-off-for-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
    class RecordingOwnerAmbassador final : public NullFederateAmbassador {
     public:
      void turnUpdatesOnForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes,
          std::wstring const& updateRateDesignator) override {
        ++turnedOnCount;
        turnedOnObjectInstance = objectInstance;
        turnedOnAttributes = attributes;
        turnedOnUpdateRateDesignator = updateRateDesignator;
      }

      void turnUpdatesOffForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes) override {
        ++turnedOffCount;
        turnedOffObjectInstance = objectInstance;
        turnedOffAttributes = attributes;
      }

      std::size_t turnedOnCount = 0U;
      rti1516_2025::ObjectInstanceHandle turnedOnObjectInstance;
      rti1516_2025::AttributeHandleSet turnedOnAttributes;
      std::optional<std::wstring> turnedOnUpdateRateDesignator;
      std::size_t turnedOffCount = 0U;
      rti1516_2025::ObjectInstanceHandle turnedOffObjectInstance;
      rti1516_2025::AttributeHandleSet turnedOffAttributes;
    } ownerFederate;
    TestFederateAmbassador receiverFederate;

    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    constexpr wchar_t const* federationName =
        L"public-process-regional-attribute-relevance-execution";
    constexpr wchar_t const* objectClassName =
        L"HLAobjectRoot.Food.Drink.Soda";
    constexpr wchar_t const* attributeName = L"Flavor";
    constexpr wchar_t const* dimensionName = L"SodaFlavor";
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry,
            composedProcessDefinition(),
            ProcessFederationServiceOptions{true});
        auto senderConnection = listener->accept(
            nullptr,
            {"public-process-regional-attribute-relevance-server", 0xA601U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession sender(senderConnection);
        auto senderHandler = service.handlerFor(sender);
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
            sender,
            senderHandler,
            TransportServiceOperation::create_federation_execution,
            "The public regional advisory server lost Create.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::join_federation_execution,
            "The public regional advisory server lost owner Join.");
        auto receiverConnection = listener->accept(
            nullptr,
            {"public-process-regional-attribute-relevance-server", 0xA602U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession receiver(receiverConnection);
        auto receiverHandler = service.handlerFor(receiver);
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::join_federation_execution,
            "The public regional advisory server lost receiver Join.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::set_attribute_relevance_advisory_switch,
            "The public regional advisory server lost advisory-switch Set.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_attribute_relevance_advisory_switch,
            "The public regional advisory server lost advisory-switch Get.");

        for (auto const operation : {
                 TransportServiceOperation::get_object_class_handle,
                 TransportServiceOperation::get_attribute_handle,
                 TransportServiceOperation::get_dimension_handle,
                 TransportServiceOperation::create_region,
                 TransportServiceOperation::set_range_bounds,
                 TransportServiceOperation::commit_region_modifications,
                 TransportServiceOperation::create_region,
                 TransportServiceOperation::set_range_bounds,
                 TransportServiceOperation::commit_region_modifications,
             }) {
          serveExpected(
              sender,
              senderHandler,
              operation,
              "The public regional advisory server lost owner DDM setup.");
        }
        for (auto const operation : {
                 TransportServiceOperation::get_object_class_handle,
                 TransportServiceOperation::get_attribute_handle,
                 TransportServiceOperation::get_dimension_handle,
                 TransportServiceOperation::create_region,
                 TransportServiceOperation::set_range_bounds,
                 TransportServiceOperation::commit_region_modifications,
                 TransportServiceOperation::subscribe_object_class_attributes_with_regions,
             }) {
          serveExpected(
              receiver,
              receiverHandler,
              operation,
              "The public regional advisory server lost receiver DDM setup.");
        }
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::publish_object_class_attributes,
            "The public regional advisory server lost Publish.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::register_object_instance_with_regions,
            "The public regional advisory server lost regional Register.");
        if (callbackModel == HLA_IMMEDIATE) {
          serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::get_object_class_handle,
              "The public regional advisory server lost immediate discovery polling.");
        } else {
          serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::receive_interaction,
              "The public regional advisory server lost discovery Receive.");
        }
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::associate_regions_for_updates,
            "The public regional advisory server lost disjoint-region Associate.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::unassociate_regions_for_updates,
            "The public regional advisory server lost source-region Unassociate.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::associate_regions_for_updates,
            "The public regional advisory server lost source-region Associate.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::resign_federation_execution,
            "The public regional advisory server lost owner Resign.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::resign_federation_execution,
            "The public regional advisory server lost receiver Resign.");
        service.detach(sender);
        service.detach(receiver);
        senderConnection->close();
        receiverConnection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    auto ownerRti = makeRti();
    auto receiverRti = makeRti();
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-regional-attribute-relevance-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool ownerJoined = false;
    bool receiverJoined = false;
    try {
      REQUIRE(ownerRti->connect(ownerFederate, callbackModel, configuration).addressUsed);
      ownerRti->createFederationExecution(federationName, L"server-owned-fom.xml");
      ownerRti->joinFederationExecution(
          L"public-process-regional-attribute-relevance-owner",
          L"public-process-regional-attribute-relevance-type",
          federationName);
      ownerJoined = true;

      REQUIRE(receiverRti->connect(receiverFederate, callbackModel, configuration).addressUsed);
      receiverRti->joinFederationExecution(
          L"public-process-regional-attribute-relevance-receiver",
          L"public-process-regional-attribute-relevance-type",
          federationName);
      receiverJoined = true;
      REQUIRE_NOTHROW(ownerRti->setAttributeRelevanceAdvisorySwitch(true));
      REQUIRE(ownerRti->getAttributeRelevanceAdvisorySwitch());

      auto const ownerObjectClass =
          ownerRti->getObjectClassHandle(objectClassName);
      auto const ownerAttribute =
          ownerRti->getAttributeHandle(ownerObjectClass, attributeName);
      auto const ownerDimension = ownerRti->getDimensionHandle(dimensionName);
      REQUIRE(ownerObjectClass.isValid());
      REQUIRE(ownerAttribute.isValid());
      REQUIRE(ownerDimension.isValid());

      auto const ownerRegion =
          ownerRti->createRegion(rti1516_2025::DimensionHandleSet{ownerDimension});
      REQUIRE(ownerRegion.isValid());
      REQUIRE_NOTHROW(ownerRti->setRangeBounds(
          ownerRegion,
          ownerDimension,
          rti1516_2025::RangeBounds(0UL, 1UL)));
      REQUIRE_NOTHROW(ownerRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{ownerRegion}));
      auto const disjointOwnerRegion =
          ownerRti->createRegion(rti1516_2025::DimensionHandleSet{ownerDimension});
      REQUIRE(disjointOwnerRegion.isValid());
      REQUIRE_NOTHROW(ownerRti->setRangeBounds(
          disjointOwnerRegion,
          ownerDimension,
          rti1516_2025::RangeBounds(2UL, 3UL)));
      REQUIRE_NOTHROW(ownerRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{disjointOwnerRegion}));

      auto const receiverObjectClass =
          receiverRti->getObjectClassHandle(objectClassName);
      auto const receiverAttribute =
          receiverRti->getAttributeHandle(receiverObjectClass, attributeName);
      auto const receiverDimension = receiverRti->getDimensionHandle(dimensionName);
      REQUIRE(receiverObjectClass == ownerObjectClass);
      REQUIRE(receiverAttribute == ownerAttribute);
      REQUIRE(receiverDimension == ownerDimension);
      auto const receiverRegion =
          receiverRti->createRegion(rti1516_2025::DimensionHandleSet{receiverDimension});
      REQUIRE(receiverRegion.isValid());
      REQUIRE_NOTHROW(receiverRti->setRangeBounds(
          receiverRegion,
          receiverDimension,
          rti1516_2025::RangeBounds(0UL, 1UL)));
      REQUIRE_NOTHROW(receiverRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{receiverRegion}));

      rti1516_2025::AttributeHandleSet const ownerAttributes{ownerAttribute};
      rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const receiverPairs{{
          rti1516_2025::AttributeHandleSet{receiverAttribute},
          rti1516_2025::RegionHandleSet{receiverRegion},
      }};
      REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributesWithRegions(
          receiverObjectClass,
          receiverPairs,
          true,
          L"High"));
      REQUIRE_NOTHROW(ownerRti->publishObjectClassAttributes(
          ownerObjectClass,
          ownerAttributes));
      auto const objectInstance = ownerRti->registerObjectInstanceWithRegions(
          ownerObjectClass,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{ownerRegion},
          }});
      REQUIRE(objectInstance.isValid());
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(receiverRti->getObjectClassHandle(objectClassName));
      } else {
        static_cast<void>(receiverRti->evokeCallback(0.0));
      }

      if (callbackModel == HLA_EVOKED) {
        static_cast<void>(ownerRti->evokeCallback(0.0));
      }
      REQUIRE(ownerFederate.turnedOffCount == 0U);
      REQUIRE(ownerFederate.turnedOnCount == 1U);
      REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOnAttributes == ownerAttributes);
      REQUIRE(ownerFederate.turnedOnUpdateRateDesignator ==
              std::optional<std::wstring>{L"High"});
      ownerFederate.turnedOnCount = 0U;
      ownerFederate.turnedOnObjectInstance = {};
      ownerFederate.turnedOnAttributes.clear();
      ownerFederate.turnedOnUpdateRateDesignator.reset();

      // A disjoint association does not change effective relevance while the
      // original overlap remains active.
      REQUIRE_NOTHROW(ownerRti->associateRegionsForUpdates(
          objectInstance,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{disjointOwnerRegion},
          }}));
      REQUIRE(ownerFederate.turnedOffCount == 0U);
      REQUIRE(ownerFederate.turnedOnCount == 0U);

      // Removing the last overlapping association crosses the edge to Off.
      REQUIRE_NOTHROW(ownerRti->unassociateRegionsForUpdates(
          objectInstance,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{ownerRegion},
          }}));
      if (callbackModel == HLA_EVOKED) {
        static_cast<void>(ownerRti->evokeCallback(0.0));
      }
      REQUIRE(ownerFederate.turnedOffCount == 1U);
      REQUIRE(ownerFederate.turnedOffObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOffAttributes == ownerAttributes);
      REQUIRE(ownerFederate.turnedOnCount == 0U);

      // Restoring the overlap crosses the edge to On and retains High.
      REQUIRE_NOTHROW(ownerRti->associateRegionsForUpdates(
          objectInstance,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{ownerRegion},
          }}));
      if (callbackModel == HLA_EVOKED) {
        static_cast<void>(ownerRti->evokeCallback(0.0));
      }
      REQUIRE(ownerFederate.turnedOnCount == 1U);
      REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOnAttributes == ownerAttributes);
      REQUIRE(ownerFederate.turnedOnUpdateRateDesignator ==
              std::optional<std::wstring>{L"High"});

      ownerRti->resignFederationExecution(DELETE_OBJECTS);
      ownerJoined = false;
      receiverRti->resignFederationExecution(NO_ACTION);
      receiverJoined = false;
      ownerRti->disconnect();
      receiverRti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      if (ownerJoined) {
        try {
          ownerRti->resignFederationExecution(DELETE_OBJECTS);
        } catch (...) {
        }
      }
      if (receiverJoined) {
        try {
          receiverRti->resignFederationExecution(NO_ACTION);
        } catch (...) {
        }
      }
      try {
        ownerRti->disconnect();
      } catch (...) {
      }
      try {
        receiverRti->disconnect();
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
    REQUIRE_FALSE(ownerJoined);
    REQUIRE_FALSE(receiverJoined);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}
#endif

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassador rejects an incompatible logical time through a configured process endpoint",
    "[integration][foundation][time-management][time-advance][transport][process-boundary][public-endpoint][rti.service.time-advance-request][rti.error.invalid-logical-time]") {
  constexpr wchar_t const* federationName =
      L"process-time-advance-invalid-time-execution";
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto connection = listener->accept(
          nullptr,
          {"process-time-advance-invalid-time-server", 0x9206U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return umbra::test::servePrimaryProcessRequest(
            session, handler,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The process invalid-time server received an unexpected operation.");
              }
              return handler(request);
            });
      };
      if (!serveExpected(TransportServiceOperation::create_federation_execution) ||
          !serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::time_advance_request) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The process invalid-time server lost a temporal request.");
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessTimeFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"process-time-advance-invalid-time-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  try {
    auto const connectionResult = rti->connect(federate, HLA_EVOKED, configuration);
    REQUIRE(connectionResult.addressUsed);
    REQUIRE_NOTHROW(rti->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    auto const joined = rti->joinFederationExecution(
        L"process-time-advance-invalid-time-type", federationName);
    REQUIRE(joined.isValid());
    REQUIRE_THROWS_AS(
        rti->timeAdvanceRequest(rti1516_2025::HLAfloat64Time(4.0)),
        rti1516_2025::InvalidLogicalTime);
    REQUIRE(federate.timeAdvanceGrantCount == 0U);
    REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
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
TEST_CASE(
    "RTIambassador expands process class Request Attribute Value Update across registered instances",
    "[integration][foundation][object-management][callbacks][callback-controls][transport][process-boundary][public-endpoint][rti.service.request-attribute-value-update][federate.callback.discover-object-instance][federate.callback.provide-attribute-value-update]") {
  auto runScenario = [](CallbackModel callbackModel) {
    class RecordingFederateAmbassador final : public NullFederateAmbassador {
     public:
      void discoverObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::ObjectClassHandle const& objectClass,
          std::wstring const& objectInstanceName,
          rti1516_2025::FederateHandle const& producingFederate) override {
        discoveredObjects.push_back(objectInstance);
        discoveredClasses.push_back(objectClass);
        discoveredNames.push_back(objectInstanceName);
        discoveredProducers.push_back(producingFederate);
      }

      void provideAttributeValueUpdate(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes,
          rti1516_2025::VariableLengthData const& userSuppliedTag) override {
        providedObjects.push_back(objectInstance);
        providedAttributeCounts.push_back(attributes.size());
        providedTags.emplace_back();
        auto& tag = providedTags.back();
        if (userSuppliedTag.size() != 0U) {
          auto const* first = static_cast<std::uint8_t const*>(
              userSuppliedTag.data());
          tag.assign(first, first + userSuppliedTag.size());
        }
      }

      std::vector<rti1516_2025::ObjectInstanceHandle> discoveredObjects;
      std::vector<rti1516_2025::ObjectClassHandle> discoveredClasses;
      std::vector<std::wstring> discoveredNames;
      std::vector<rti1516_2025::FederateHandle> discoveredProducers;
      std::vector<rti1516_2025::ObjectInstanceHandle> providedObjects;
      std::vector<std::size_t> providedAttributeCounts;
      std::vector<std::vector<std::uint8_t>> providedTags;
    } providerFederate, requesterFederate;

    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    constexpr wchar_t const* federationNameWide =
        L"public-process-class-attribute-execution";
    constexpr wchar_t const* objectClassNameWide = L"HLAobjectRoot.Employee";
    constexpr char const* objectClassName = "HLAobjectRoot.Employee";
    constexpr wchar_t const* attributeNameWide = L"Name";
    constexpr char const* attributeName = "Name";
    std::atomic_uint64_t expectedObjectClass{0U};
    std::atomic_uint64_t expectedAttribute{0U};
    std::atomic_uint64_t expectedProviderFederate{0U};
    std::atomic_uint64_t expectedRequesterFederate{0U};
    std::atomic_uint64_t recipientCount{
        std::numeric_limits<std::uint32_t>::max()};
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry,
            composedProcessDefinition(),
            ProcessFederationServiceOptions{callbackModel == HLA_IMMEDIATE});
        auto providerConnection = listener->accept(
            nullptr,
            {"public-process-class-attribute-server", 0x9A11U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession provider(providerConnection);
        auto providerBaseHandler = service.handlerFor(provider);
        auto providerHandler = [&](TransportServiceMessage const& request) {
          auto response = providerBaseHandler(request);
          if (request.operation ==
                  TransportServiceOperation::join_federation_execution &&
              response.status == TransportServiceStatus::ok) {
            auto const join = umbra::detail::decodeProcessFederationJoinResult(
                response.payload);
            expectedProviderFederate.store(
                join.federateId, std::memory_order_release);
          }
          return response;
        };
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
            provider,
            providerHandler,
            TransportServiceOperation::create_federation_execution,
            "The public process class server lost Create.");
        auto const objectClass = registry.objectClassHandleFor(
            federationNameWide, objectClassName);
        auto const attribute = registry.attributeHandleFor(
            federationNameWide, objectClassName, attributeName);
        if (!objectClass || !attribute) {
          throw std::runtime_error(
              "The public process class server could not resolve its FOM handles.");
        }
        expectedObjectClass.store(*objectClass, std::memory_order_release);
        expectedAttribute.store(*attribute, std::memory_order_release);
        serveExpected(
            provider,
            providerHandler,
            TransportServiceOperation::join_federation_execution,
            "The public process class server lost provider Join.");

        auto requesterConnection = listener->accept(
            nullptr,
            {"public-process-class-attribute-server", 0x9A12U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession requester(requesterConnection);
        auto requesterBaseHandler = service.handlerFor(requester);
        auto requesterHandler = [&](TransportServiceMessage const& request) {
          auto response = requesterBaseHandler(request);
          if (request.operation ==
                  TransportServiceOperation::join_federation_execution &&
              response.status == TransportServiceStatus::ok) {
            auto const join = umbra::detail::decodeProcessFederationJoinResult(
                response.payload);
            expectedRequesterFederate.store(
                join.federateId, std::memory_order_release);
          }
          if (request.operation ==
                  TransportServiceOperation::request_attribute_value_update_class &&
              response.status == TransportServiceStatus::ok) {
            recipientCount.store(
                umbra::detail::decodeProcessFederationRequestAttributeValueUpdateResult(
                    response.payload)
                    .recipientCount,
                std::memory_order_release);
          }
          return response;
        };
        serveExpected(
            requester,
            requesterHandler,
            TransportServiceOperation::join_federation_execution,
            "The public process class server lost requester Join.");

        auto const providerMember = registry.memberByName(
            federationNameWide, L"public-process-class-attribute-provider");
        auto const requesterMember = registry.memberByName(
            federationNameWide, L"public-process-class-attribute-requester");
        if (!providerMember || !requesterMember ||
            registry.setObjectClassAttributePublication(
                federationNameWide,
                providerMember->id,
                *objectClass,
                std::set<std::uint64_t>{*attribute},
                true) !=
                umbra::detail::ObjectClassAttributeDeclarationStatus::applied ||
            registry.setObjectClassAttributeSubscription(
                federationNameWide,
                requesterMember->id,
                *objectClass,
                std::set<std::uint64_t>{*attribute},
                true) !=
                umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
          throw std::runtime_error(
              "The public process class declarations were rejected.");
        }
        serveExpected(
            provider,
            providerHandler,
            TransportServiceOperation::get_object_class_handle,
            "The public process class server lost provider class lookup.");
        serveExpected(
            provider,
            providerHandler,
            TransportServiceOperation::get_attribute_handle,
            "The public process class server lost provider attribute lookup.");
        serveExpected(
            requester,
            requesterHandler,
            TransportServiceOperation::get_object_class_handle,
            "The public process class server lost requester class lookup.");
        serveExpected(
            requester,
            requesterHandler,
            TransportServiceOperation::get_attribute_handle,
            "The public process class server lost requester attribute lookup.");
        serveExpected(
            provider,
            providerHandler,
            TransportServiceOperation::publish_object_class_attributes,
            "The public process class server lost Publish.");
        for (std::size_t index = 0U; index < 2U; ++index) {
          serveExpected(
              provider,
              providerHandler,
              TransportServiceOperation::register_object_instance,
              "The public process class server lost Register.");
          if (callbackModel == HLA_IMMEDIATE) {
            serveExpected(
                provider,
                providerHandler,
                TransportServiceOperation::get_object_class_handle,
                "The public process class server lost immediate advisory flush.");
            serveExpected(
                requester,
                requesterHandler,
                TransportServiceOperation::get_object_class_handle,
                "The public process class server lost immediate discovery flush.");
          } else {
            serveExpected(
                provider,
                providerHandler,
                TransportServiceOperation::receive_interaction,
                "The public process class server lost advisory polling.");
            serveExpected(
                requester,
                requesterHandler,
                TransportServiceOperation::receive_interaction,
                "The public process class server lost discovery polling.");
          }
        }
        serveExpected(
            requester,
            requesterHandler,
            TransportServiceOperation::request_attribute_value_update_class,
            "The public process class server lost class Request Attribute Value Update.");
        for (std::size_t index = 0U; index < 2U; ++index) {
          if (callbackModel == HLA_IMMEDIATE) {
            serveExpected(
                provider,
                providerHandler,
                TransportServiceOperation::get_object_class_handle,
                "The public process class server lost immediate Provide flush.");
          } else {
            serveExpected(
                provider,
                providerHandler,
                TransportServiceOperation::receive_interaction,
                "The public process class server lost Provide polling.");
          }
        }
        serveExpected(
            provider,
            providerHandler,
            TransportServiceOperation::resign_federation_execution,
            "The public process class server lost provider Resign.");
        serveExpected(
            requester,
            requesterHandler,
            TransportServiceOperation::resign_federation_execution,
            "The public process class server lost requester Resign.");
        service.detach(provider);
        service.detach(requester);
        providerConnection->close();
        requesterConnection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    auto providerRti = makeRti();
    auto requesterRti = makeRti();
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-class-attribute-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool providerJoined = false;
    bool requesterJoined = false;
    std::vector<rti1516_2025::ObjectInstanceHandle> objectInstances;
    try {
      REQUIRE(providerRti->connect(providerFederate, callbackModel, configuration)
                  .addressUsed);
      providerRti->createFederationExecution(
          federationNameWide, L"server-owned-fom.xml");
      providerRti->joinFederationExecution(
          L"public-process-class-attribute-provider",
          L"public-process-class-attribute-type",
          federationNameWide);
      providerJoined = true;

      REQUIRE(requesterRti->connect(requesterFederate, callbackModel, configuration)
                  .addressUsed);
      requesterRti->joinFederationExecution(
          L"public-process-class-attribute-requester",
          L"public-process-class-attribute-type",
          federationNameWide);
      requesterJoined = true;

      auto const providerObjectClass =
          providerRti->getObjectClassHandle(objectClassNameWide);
      auto const providerAttribute =
          providerRti->getAttributeHandle(providerObjectClass, attributeNameWide);
      auto const requesterObjectClass =
          requesterRti->getObjectClassHandle(objectClassNameWide);
      auto const requesterAttribute =
          requesterRti->getAttributeHandle(requesterObjectClass, attributeNameWide);
      REQUIRE(providerObjectClass.toString() ==
              L"ObjectClassHandle(" +
                  std::to_wstring(expectedObjectClass.load(
                      std::memory_order_acquire)) +
                  L")");
      REQUIRE(providerAttribute ==
              rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                  expectedAttribute.load(std::memory_order_acquire)));
      REQUIRE(requesterObjectClass == providerObjectClass);
      REQUIRE(requesterAttribute == providerAttribute);

      rti1516_2025::AttributeHandleSet publishedAttributes;
      publishedAttributes.insert(providerAttribute);
      providerRti->publishObjectClassAttributes(
          providerObjectClass, publishedAttributes);
      for (std::size_t index = 0U; index < 2U; ++index) {
        auto const objectInstance =
            providerRti->registerObjectInstance(providerObjectClass);
        REQUIRE(objectInstance.isValid());
        objectInstances.push_back(objectInstance);
        if (callbackModel == HLA_IMMEDIATE) {
          static_cast<void>(providerRti->getObjectClassHandle(objectClassNameWide));
          static_cast<void>(requesterRti->getObjectClassHandle(objectClassNameWide));
        } else {
          static_cast<void>(providerRti->evokeCallback(0.0));
          static_cast<void>(requesterRti->evokeCallback(0.0));
        }
      }
      REQUIRE(requesterFederate.discoveredObjects.size() == 2U);
      REQUIRE(requesterFederate.discoveredObjects[0] == objectInstances[0]);
      REQUIRE(requesterFederate.discoveredObjects[1] == objectInstances[1]);

      rti1516_2025::AttributeHandleSet requestedAttributes;
      requestedAttributes.insert(requesterAttribute);
      std::array<std::uint8_t, 3U> encodedTag{0x43U, 0x4CU, 0x53U};
      VariableLengthData userSuppliedTag(encodedTag.data(), encodedTag.size());
      REQUIRE_NOTHROW(requesterRti->requestAttributeValueUpdate(
          requesterObjectClass, requestedAttributes, userSuppliedTag));
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(providerRti->getObjectClassHandle(objectClassNameWide));
        static_cast<void>(providerRti->getObjectClassHandle(objectClassNameWide));
      } else {
        static_cast<void>(providerRti->evokeCallback(0.0));
        static_cast<void>(providerRti->evokeCallback(0.0));
      }
      REQUIRE(providerFederate.providedObjects.size() == 2U);
      REQUIRE(providerFederate.providedObjects[0] == objectInstances[0]);
      REQUIRE(providerFederate.providedObjects[1] == objectInstances[1]);
      REQUIRE(providerFederate.providedAttributeCounts ==
              std::vector<std::size_t>{1U, 1U});
      REQUIRE(providerFederate.providedTags ==
              std::vector<std::vector<std::uint8_t>>{
                  {0x43U, 0x4CU, 0x53U}, {0x43U, 0x4CU, 0x53U}});

      providerRti->resignFederationExecution(DELETE_OBJECTS);
      providerJoined = false;
      requesterRti->resignFederationExecution(NO_ACTION);
      requesterJoined = false;
      providerRti->disconnect();
      requesterRti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      if (providerJoined) {
        try {
          providerRti->resignFederationExecution(DELETE_OBJECTS);
        } catch (...) {
        }
      }
      if (requesterJoined) {
        try {
          requesterRti->resignFederationExecution(NO_ACTION);
        } catch (...) {
        }
      }
      try {
        providerRti->disconnect();
      } catch (...) {
      }
      try {
        requesterRti->disconnect();
      } catch (...) {
      }
    }
    listener.reset();
    if (server.joinable()) {
      server.join();
    }
    if (clientError) {
      if (serverError) {
        std::rethrow_exception(serverError);
      }
      std::rethrow_exception(clientError);
    }
    REQUIRE_FALSE(serverError);
    REQUIRE(recipientCount.load(std::memory_order_acquire) == 2U);
    REQUIRE(expectedProviderFederate.load(std::memory_order_acquire) != 0U);
    REQUIRE(expectedRequesterFederate.load(std::memory_order_acquire) != 0U);
    REQUIRE_FALSE(providerJoined);
    REQUIRE_FALSE(requesterJoined);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#endif
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)






#endif
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

#endif
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#endif
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#endif

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)


TEST_CASE(
    "RTIambassadors preserve FIFO order when a middle timestamped process attribute is retracted before the callback",
    "[integration][foundation][object-management][time-management][time-advance]"
    "[transport][process-boundary][public-endpoint][multi-federate][tso][retraction]"
    "[timestamped-process-attribute-update][process-tso-attribute-retraction-ordering]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const&,
        std::wstring const&,
        rti1516_2025::FederateHandle const&) override {
      discoveredObjectInstance = objectInstance;
      discovered = true;
    }

    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const&,
        rti1516_2025::TransportationTypeHandle const&,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const*,
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::OrderType sentOrder,
        rti1516_2025::OrderType receivedOrder,
        rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
      ++reflectedCount;
      reflectedObjectInstance = objectInstance;
      reflectedAttributeCounts.push_back(attributeValues.size());
      reflectedProducingFederate = producingFederate;
      reflectedSentOrders.push_back(sentOrder);
      reflectedReceivedOrders.push_back(receivedOrder);
      reflectedHasRetraction.push_back(
          optionalRetraction != nullptr && optionalRetraction->isValid());
      auto const* integerTime =
          dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time);
      REQUIRE(integerTime != nullptr);
      reflectedTimestampValues.push_back(integerTime->getTime());
      callbackOrder.push_back('R');
    }

    void timeRegulationEnabled(rti1516_2025::LogicalTime const&) override {
      ++timeRegulationEnabledCount;
    }

    void timeConstrainedEnabled(rti1516_2025::LogicalTime const&) override {
      ++timeConstrainedEnabledCount;
    }

    void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
      ++timeAdvanceGrantCount;
      callbackOrder.push_back('G');
      auto const* integerTime =
          dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time);
      REQUIRE(integerTime != nullptr);
      timeAdvanceGrantValue = integerTime->getTime();
    }

    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    std::size_t reflectedCount = 0U;
    rti1516_2025::ObjectInstanceHandle reflectedObjectInstance;
    std::vector<std::size_t> reflectedAttributeCounts;
    rti1516_2025::FederateHandle reflectedProducingFederate;
    std::vector<rti1516_2025::OrderType> reflectedSentOrders;
    std::vector<rti1516_2025::OrderType> reflectedReceivedOrders;
    std::vector<bool> reflectedHasRetraction;
    std::vector<std::int64_t> reflectedTimestampValues;
    std::size_t timeRegulationEnabledCount = 0U;
    std::size_t timeConstrainedEnabledCount = 0U;
    std::size_t timeAdvanceGrantCount = 0U;
    std::int64_t timeAdvanceGrantValue = -1;
    std::vector<char> callbackOrder;
  } senderFederate, receiverFederate;

  constexpr wchar_t const* federationName =
      L"process-timestamped-attribute-retraction-ordering-execution";
  constexpr wchar_t const* senderName =
      L"process-timestamped-attribute-retraction-ordering-sender";
  constexpr wchar_t const* receiverName =
      L"process-timestamped-attribute-retraction-ordering-receiver";
  constexpr wchar_t const* objectClassName = L"HLAobjectRoot.Employee";
  constexpr wchar_t const* attributeName = L"Name";

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"process-timestamped-attribute-retraction-ordering-server", 0x9350U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto const senderHandler = service.handlerFor(sender);
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
          sender,
          senderHandler,
          TransportServiceOperation::create_federation_execution,
          "The process ordering server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationName, "HLAobjectRoot.Employee");
      auto const attribute = registry.attributeHandleFor(
          federationName, "HLAobjectRoot.Employee", "Name");
      if (!objectClass || !attribute) {
        throw std::runtime_error(
            "The process ordering server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The process ordering server lost sender Join.");

      auto receiverConnection = listener->accept(
          nullptr,
          {"process-timestamped-attribute-retraction-ordering-server", 0x9351U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto const receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The process ordering server lost receiver Join.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process ordering server lost sender object lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process ordering server lost receiver object lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process ordering server lost sender attribute lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process ordering server lost receiver attribute lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process ordering server lost Publish.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The process ordering server lost Subscribe.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::enable_time_regulation,
          "The process ordering server lost Enable Time Regulation.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::enable_time_constrained,
          "The process ordering server lost Enable Time Constrained.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::register_object_instance,
          "The process ordering server lost Register.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The process ordering server lost discovery polling.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::update_attribute_values,
          "The process ordering server lost timestamp-5 Update.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::update_attribute_values,
          "The process ordering server lost timestamp-6 Update.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::update_attribute_values,
          "The process ordering server lost timestamp-7 Update.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::retract,
          "The process ordering server lost middle Retract.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::time_advance_request,
          "The process ordering server lost receiver TAR at timestamp 5.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The process ordering server lost sender intermediate TAR.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The process ordering server lost sender timestamp-5 TAR.");
      std::size_t receiverPolls = 0U;
      std::size_t receiverAcknowledgements = 0U;
      auto serveReceiverUntilAcknowledgements =
          [&](std::size_t target, char const* description) {
            while (receiverAcknowledgements < target && receiverPolls != 8U) {
              if (!umbra::test::servePrimaryProcessRequest(
                      receiver, receiverHandler,
                      [&](TransportServiceMessage const& request) {
                        if (request.operation ==
                            TransportServiceOperation::receive_interaction) {
                          ++receiverPolls;
                          return receiverHandler(request);
                        }
                        if (request.operation ==
                            TransportServiceOperation::acknowledge_tso_delivery) {
                          ++receiverAcknowledgements;
                          return receiverHandler(request);
                        }
                        throw std::runtime_error(
                            std::string(
                                "The process ordering server received an unexpected receiver callback operation ") +
                            std::to_string(static_cast<unsigned>(request.operation)) +
                            ".");
                      })) {
                throw std::runtime_error(
                    "The process ordering server lost a receiver callback operation.");
              }
            }
            if (receiverAcknowledgements != target) {
              throw std::runtime_error(description);
            }
      };
      serveReceiverUntilAcknowledgements(
          1U,
          "The process ordering server did not receive the timestamp-5 acknowledgement.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::time_advance_request,
          "The process ordering server lost receiver TAR at timestamp 7.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The process ordering server lost sender final TAR.");
      serveReceiverUntilAcknowledgements(
          2U,
          "The process ordering server did not receive the timestamp-7 acknowledgement.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process ordering server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process ordering server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto senderConfiguration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"process-timestamped-attribute-retraction-ordering-sender-client")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto receiverConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"process-timestamped-attribute-retraction-ordering-receiver-client")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(
                senderFederate, HLA_EVOKED, senderConfiguration)
                .addressUsed);
    REQUIRE_NOTHROW(senderRti->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    auto const senderHandle = senderRti->joinFederationExecution(
        senderName,
        L"process-timestamped-attribute-retraction-ordering-type",
        federationName);
    REQUIRE(senderHandle.isValid());
    senderJoined = true;
    REQUIRE(receiverRti->connect(
                receiverFederate, HLA_EVOKED, receiverConfiguration)
                .addressUsed);
    auto const receiverHandle = receiverRti->joinFederationExecution(
        receiverName,
        L"process-timestamped-attribute-retraction-ordering-type",
        federationName);
    REQUIRE(receiverHandle.isValid());
    receiverJoined = true;

    auto const senderObjectClass = senderRti->getObjectClassHandle(objectClassName);
    auto const receiverObjectClass = receiverRti->getObjectClassHandle(objectClassName);
    auto const senderAttribute = senderRti->getAttributeHandle(
        senderObjectClass, attributeName);
    auto const receiverAttribute = receiverRti->getAttributeHandle(
        receiverObjectClass, attributeName);
    REQUIRE(senderObjectClass == receiverObjectClass);
    REQUIRE(senderObjectClass ==
            rti1516_2025::umbra_binding_detail::makeObjectClassHandle(
                expectedObjectClass.load(std::memory_order_acquire)));
    REQUIRE(senderAttribute == receiverAttribute);
    REQUIRE(senderAttribute ==
            rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                expectedAttribute.load(std::memory_order_acquire)));
    REQUIRE_NOTHROW(senderRti->publishObjectClassAttributes(
        senderObjectClass, {senderAttribute}));
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributes(
        receiverObjectClass, {receiverAttribute}, true));

    REQUIRE_NOTHROW(senderRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE(senderFederate.timeRegulationEnabledCount == 0U);
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeRegulationEnabledCount == 1U);
    REQUIRE_NOTHROW(receiverRti->enableTimeConstrained());
    REQUIRE(receiverFederate.timeConstrainedEnabledCount == 0U);
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.timeConstrainedEnabledCount == 1U);

    auto const objectInstance = senderRti->registerObjectInstance(senderObjectClass);
    REQUIRE(objectInstance.isValid());
    REQUIRE_FALSE(receiverFederate.discovered);
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.discovered);
    REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);

    AttributeHandleValueMap attributeValues;
    std::array<std::uint8_t, 1U> valueBytes{0U};
    attributeValues.emplace(
        senderAttribute,
        VariableLengthData(valueBytes.data(), valueBytes.size()));
    std::array<std::uint8_t, 1U> tagBytes{0U};
    auto sendAt = [&](std::uint8_t value, std::uint8_t tag, std::int64_t time) {
      valueBytes[0] = value;
      tagBytes[0] = tag;
      auto const retraction = senderRti->updateAttributeValues(
          objectInstance,
          attributeValues,
          VariableLengthData(tagBytes.data(), tagBytes.size()),
          rti1516_2025::HLAinteger64Time(time));
      REQUIRE(retraction.isValid());
      return retraction;
    };
    auto const first = sendAt(0x35U, 0xA5U, 5);
    auto const middle = sendAt(0x36U, 0xA6U, 6);
    auto const last = sendAt(0x37U, 0xA7U, 7);
    REQUIRE(first.isValid());
    REQUIRE(middle.isValid());
    REQUIRE(last.isValid());
    REQUIRE_NOTHROW(senderRti->retract(middle));
    REQUIRE(receiverFederate.reflectedCount == 0U);
    REQUIRE_NOTHROW(receiverRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE(senderFederate.timeAdvanceGrantCount == 0U);
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeAdvanceGrantCount == 1U);
    // The sender requested time 5, so its own grant is 5.  The constrained
    // receiver reaches the same timestamp independently; the retracted
    // middle update does not lower the sender's TAR target.
    REQUIRE(senderFederate.timeAdvanceGrantValue == 5);
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE(senderFederate.timeAdvanceGrantCount == 1U);
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeAdvanceGrantCount == 2U);
    REQUIRE(senderFederate.timeAdvanceGrantValue == 5);

    // A constrained grant is released at the earliest queued TSO boundary.
    // The first callback fence therefore admits timestamp 5; the later
    // request to 7 admits timestamp 7 after the middle update is retracted.
    for (int pass = 0; pass != 8 &&
         (receiverFederate.reflectedCount < 1U ||
          receiverFederate.timeAdvanceGrantCount < 1U);
         ++pass) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.reflectedCount == 1U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(receiverFederate.timeAdvanceGrantValue == 5);
    REQUIRE(receiverFederate.reflectedTimestampValues ==
            std::vector<std::int64_t>{5});
    REQUIRE(receiverFederate.callbackOrder == std::vector<char>{'R', 'G'});

    REQUIRE_NOTHROW(receiverRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 1U);
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    REQUIRE(senderFederate.timeAdvanceGrantCount == 2U);
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeAdvanceGrantCount == 3U);
    REQUIRE(senderFederate.timeAdvanceGrantValue == 7);
    for (int pass = 0; pass != 8 &&
         (receiverFederate.reflectedCount < 2U ||
          receiverFederate.timeAdvanceGrantCount < 2U);
         ++pass) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.reflectedCount == 2U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 2U);
    REQUIRE(receiverFederate.timeAdvanceGrantValue == 7);
    REQUIRE(receiverFederate.reflectedTimestampValues ==
            std::vector<std::int64_t>{5, 7});
    REQUIRE(receiverFederate.callbackOrder ==
            std::vector<char>{'R', 'G', 'R', 'G'});
    REQUIRE(receiverFederate.reflectedAttributeCounts ==
            std::vector<std::size_t>{1U, 1U});
    REQUIRE(receiverFederate.reflectedSentOrders ==
            std::vector<rti1516_2025::OrderType>{rti1516_2025::TIMESTAMP,
                                                  rti1516_2025::TIMESTAMP});
    REQUIRE(receiverFederate.reflectedReceivedOrders ==
            std::vector<rti1516_2025::OrderType>{rti1516_2025::TIMESTAMP,
                                                  rti1516_2025::TIMESTAMP});
    REQUIRE(receiverFederate.reflectedHasRetraction ==
            std::vector<bool>{true, true});
    REQUIRE(receiverFederate.reflectedObjectInstance == objectInstance);
    REQUIRE(receiverFederate.reflectedProducingFederate == senderHandle);

    REQUIRE_NOTHROW(senderRti->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
    senderJoined = false;
    REQUIRE_NOTHROW(receiverRti->resignFederationExecution(NO_ACTION));
    receiverJoined = false;
    REQUIRE_NOTHROW(senderRti->disconnect());
    REQUIRE_NOTHROW(receiverRti->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (receiverJoined) {
      try {
        receiverRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (senderJoined) {
      try {
        senderRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      senderRti->disconnect();
    } catch (...) {
    }
    try {
      receiverRti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (serverError) {
    try {
      std::rethrow_exception(serverError);
    } catch (std::exception const& error) {
      FAIL_CHECK(std::string("process ordering server: ") + error.what());
    } catch (...) {
      FAIL_CHECK("process ordering server failed with an unknown exception");
    }
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
}


#endif

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
class TransportationTypeFederateAmbassador final : public NullFederateAmbassador {
 public:
  void confirmAttributeTransportationTypeChange(
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      rti1516_2025::AttributeHandleSet const& attributes,
      rti1516_2025::TransportationTypeHandle const& transportationType) override {
    ++changeCount;
    changedObjectInstance = objectInstance;
    changedAttributes = attributes;
    changedTransportationType = transportationType;
  }

  void reportAttributeTransportationType(
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      rti1516_2025::AttributeHandle const& attribute,
      rti1516_2025::TransportationTypeHandle const& transportationType) override {
    ++queryCount;
    queriedObjectInstance = objectInstance;
    queriedAttribute = attribute;
    queriedTransportationType = transportationType;
  }

  std::size_t changeCount = 0U;
  rti1516_2025::ObjectInstanceHandle changedObjectInstance;
  rti1516_2025::AttributeHandleSet changedAttributes;
  rti1516_2025::TransportationTypeHandle changedTransportationType;
  std::size_t queryCount = 0U;
  rti1516_2025::ObjectInstanceHandle queriedObjectInstance;
  rti1516_2025::AttributeHandle queriedAttribute;
  rti1516_2025::TransportationTypeHandle queriedTransportationType;
};

TEST_CASE(
    "RTIambassador routes instance transportation type change and query through a configured process endpoint",
    "[integration][foundation][object-management][transportation][transport][process-boundary][public-endpoint][process-transportation-instance-control][rti.service.request-attribute-transportation-type-change][rti.service.query-attribute-transportation-type]") {
  auto runScenario = [](CallbackModel callbackModel) {
    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    constexpr wchar_t const* federationName =
        L"public-process-transportation-instance-execution";
    constexpr wchar_t const* federateName =
        L"public-process-transportation-instance-federate";
    constexpr char const* objectName = "HLAobjectRoot.Customer";
    constexpr char const* attributeName = "HLAprivilegeToDeleteObject";
    constexpr char const* transportationTypeName = "HLAreliable";
    std::atomic_uint64_t expectedObjectClass{0U};
    std::atomic_uint64_t expectedAttribute{0U};
    std::atomic_uint64_t expectedTransportation{0U};
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
        auto connection = listener->accept(
            nullptr,
            {"public-process-transportation-instance-server", 0x9603U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession session(connection);
        auto handler = service.handlerFor(session);
        auto serveExpected = [&](TransportServiceOperation operation) {
          return umbra::test::servePrimaryProcessRequest(
              session, handler,
              [&](TransportServiceMessage const& request) {
                if (request.operation != operation) {
                  throw std::runtime_error(
                      "The public process transportation-instance server received an unexpected operation (expected " +
                      std::to_string(static_cast<unsigned>(operation)) +
                      ", got " +
                      std::to_string(static_cast<unsigned>(request.operation)) + ").");
                }
                return handler(request);
              });
        };

        if (!serveExpected(TransportServiceOperation::create_federation_execution)) {
          throw std::runtime_error(
              "The public process transportation-instance server lost Create.");
        }
        auto const objectClass = registry.objectClassHandleFor(
            federationName, objectName);
        auto const attribute = registry.attributeHandleFor(
            federationName, objectName, attributeName);
        auto const transportation = registry.transportationTypeHandleFor(
            federationName, transportationTypeName);
        if (!objectClass || !attribute || !transportation) {
          throw std::runtime_error(
              "The public process transportation-instance server could not resolve its FOM handles.");
        }
        expectedObjectClass.store(*objectClass, std::memory_order_release);
        expectedAttribute.store(*attribute, std::memory_order_release);
        expectedTransportation.store(*transportation, std::memory_order_release);
        if (!serveExpected(TransportServiceOperation::join_federation_execution) ||
            !serveExpected(TransportServiceOperation::get_object_class_handle) ||
            !serveExpected(TransportServiceOperation::get_attribute_handle) ||
            !serveExpected(TransportServiceOperation::get_transportation_type_handle) ||
            !serveExpected(TransportServiceOperation::publish_object_class_attributes) ||
            !serveExpected(TransportServiceOperation::register_object_instance) ||
            !serveExpected(
                TransportServiceOperation::request_attribute_transportation_type_change) ||
            !serveExpected(TransportServiceOperation::receive_interaction) ||
            !serveExpected(TransportServiceOperation::query_attribute_transportation_type) ||
            !serveExpected(TransportServiceOperation::receive_interaction) ||
            !serveExpected(TransportServiceOperation::resign_federation_execution)) {
          throw std::runtime_error(
              "The public process transportation-instance server lost a declaration, callback, or Resign operation.");
        }
        service.detach(session);
        connection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    TransportationTypeFederateAmbassador federate;
    auto rti = makeRti();
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-transportation-instance-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool clientJoined = false;
    try {
      REQUIRE(rti->connect(federate, callbackModel, configuration).addressUsed);
      rti->createFederationExecution(federationName, L"server-owned-fom.xml");
      static_cast<void>(rti->joinFederationExecution(
          federateName,
          L"public-process-transportation-instance-type",
          federationName));
      clientJoined = true;
      auto const objectClass = rti->getObjectClassHandle(L"HLAobjectRoot.Customer");
      REQUIRE(objectClass.toString() ==
              L"ObjectClassHandle(" +
                  std::to_wstring(
                      expectedObjectClass.load(std::memory_order_acquire)) +
                  L")");
      auto const attribute =
          rti->getAttributeHandle(objectClass, L"HLAprivilegeToDeleteObject");
      REQUIRE(attribute.isValid());
      auto const transportation =
          rti->getTransportationTypeHandle(L"HLAreliable");
      REQUIRE(transportation.toString() ==
              L"TransportationTypeHandle(" +
                  std::to_wstring(
                      expectedTransportation.load(std::memory_order_acquire)) +
                  L")");
      rti1516_2025::AttributeHandleSet attributes;
      attributes.insert(attribute);
      rti->publishObjectClassAttributes(objectClass, attributes);
      auto const objectInstance = rti->registerObjectInstance(objectClass);
      REQUIRE(objectInstance.isValid());
      REQUIRE_NOTHROW(rti->requestAttributeTransportationTypeChange(
          objectInstance, attributes, transportation));
      REQUIRE_NOTHROW(rti->evokeCallback(0.0));
      REQUIRE(federate.changeCount == 1U);
      REQUIRE(federate.changedObjectInstance == objectInstance);
      REQUIRE(federate.changedAttributes.size() == 1U);
      REQUIRE(federate.changedAttributes.contains(attribute));
      REQUIRE(federate.changedTransportationType == transportation);
      REQUIRE_NOTHROW(rti->queryAttributeTransportationType(
          objectInstance, attribute));
      REQUIRE_NOTHROW(rti->evokeCallback(0.0));
      REQUIRE(federate.queryCount == 1U);
      REQUIRE(federate.queriedObjectInstance == objectInstance);
      REQUIRE(federate.queriedAttribute == attribute);
      REQUIRE(federate.queriedTransportationType == transportation);
      rti->resignFederationExecution(NO_ACTION);
      rti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      if (clientJoined) {
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
  REQUIRE(expectedAttribute.load(std::memory_order_acquire) != 0U);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}

#endif

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
class InteractionTransportationFederateAmbassador final
    : public NullFederateAmbassador {
 public:
  void confirmInteractionTransportationTypeChange(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::TransportationTypeHandle const& transportationType) override {
    ++changeCount;
    changedInteractionClass = interactionClass;
    changedTransportationType = transportationType;
  }

  void reportInteractionTransportationType(
      rti1516_2025::FederateHandle const& federate,
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::TransportationTypeHandle const& transportationType) override {
    ++queryCount;
    queriedFederate = federate;
    queriedInteractionClass = interactionClass;
    queriedTransportationType = transportationType;
  }

  std::size_t changeCount = 0U;
  rti1516_2025::InteractionClassHandle changedInteractionClass;
  rti1516_2025::TransportationTypeHandle changedTransportationType;
  std::size_t queryCount = 0U;
  rti1516_2025::FederateHandle queriedFederate;
  rti1516_2025::InteractionClassHandle queriedInteractionClass;
  rti1516_2025::TransportationTypeHandle queriedTransportationType;
};

TEST_CASE(
    "RTIambassador routes interaction transportation type change and query through a configured process endpoint",
    "[integration][foundation][interaction-management][transportation][transport][process-boundary][public-endpoint][process-transportation-interaction-control][rti.service.request-interaction-transportation-type-change][rti.service.query-interaction-transportation-type]") {
  auto runScenario = [](CallbackModel callbackModel) {
    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    constexpr wchar_t const* federationName =
        L"public-process-transportation-interaction-execution";
    constexpr wchar_t const* federateName =
        L"public-process-transportation-interaction-federate";
    constexpr char const* interactionName =
        "HLAinteractionRoot.ServerAction.TakeOrder";
    constexpr wchar_t const* interactionNameWide =
        L"HLAinteractionRoot.ServerAction.TakeOrder";
    constexpr char const* transportationTypeName = "HLAbestEffort";
    std::atomic_uint64_t expectedInteractionClass{0U};
    std::atomic_uint64_t expectedTransportation{0U};
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
        auto connection = listener->accept(
            nullptr,
            {"public-process-transportation-interaction-server", 0x9604U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession session(connection);
        auto handler = service.handlerFor(session);
        auto serveExpected = [&](TransportServiceOperation operation) {
          return umbra::test::servePrimaryProcessRequest(
              session, handler,
              [&](TransportServiceMessage const& request) {
                if (request.operation != operation) {
                  throw std::runtime_error(
                      "The public process transportation-interaction server received an unexpected operation (expected " +
                      std::to_string(static_cast<unsigned>(operation)) +
                      ", got " +
                      std::to_string(static_cast<unsigned>(request.operation)) +
                      ").");
                }
                return handler(request);
              });
        };

        if (!serveExpected(TransportServiceOperation::create_federation_execution)) {
          throw std::runtime_error(
              "The public process transportation-interaction server lost Create.");
        }
        auto const interactionClass = registry.interactionClassHandleFor(
            federationName, interactionName);
        auto const transportation = registry.transportationTypeHandleFor(
            federationName, transportationTypeName);
        if (!interactionClass || !transportation) {
          throw std::runtime_error(
              "The public process transportation-interaction server could not resolve its FOM handles.");
        }
        expectedInteractionClass.store(
            *interactionClass, std::memory_order_release);
        expectedTransportation.store(
            *transportation, std::memory_order_release);
        if (!serveExpected(TransportServiceOperation::join_federation_execution) ||
            !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
            !serveExpected(TransportServiceOperation::get_transportation_type_handle) ||
            !serveExpected(TransportServiceOperation::get_federate_handle) ||
            !serveExpected(TransportServiceOperation::publish_interaction_class) ||
            !serveExpected(
                TransportServiceOperation::request_interaction_transportation_type_change) ||
            !serveExpected(TransportServiceOperation::receive_interaction) ||
            !serveExpected(
                TransportServiceOperation::query_interaction_transportation_type) ||
            !serveExpected(TransportServiceOperation::receive_interaction) ||
            !serveExpected(TransportServiceOperation::resign_federation_execution)) {
          throw std::runtime_error(
              "The public process transportation-interaction server lost a declaration, callback, or Resign operation.");
        }
        service.detach(session);
        connection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    InteractionTransportationFederateAmbassador federate;
    auto rti = makeRti();
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-transportation-interaction-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool clientJoined = false;
    try {
      REQUIRE(rti->connect(federate, callbackModel, configuration).addressUsed);
      rti->createFederationExecution(federationName, L"server-owned-fom.xml");
      static_cast<void>(rti->joinFederationExecution(
          federateName,
          L"public-process-transportation-interaction-type",
          federationName));
      clientJoined = true;
      auto const interactionClass =
          rti->getInteractionClassHandle(interactionNameWide);
      REQUIRE(interactionClass.toString() ==
              L"InteractionClassHandle(" +
                  std::to_wstring(expectedInteractionClass.load(
                      std::memory_order_acquire)) +
                  L")");
      auto const transportation =
          rti->getTransportationTypeHandle(L"HLAbestEffort");
      REQUIRE(transportation.toString() ==
              L"TransportationTypeHandle(" +
                  std::to_wstring(expectedTransportation.load(
                      std::memory_order_acquire)) +
                  L")");
      auto const queriedFederate = rti->getFederateHandle(federateName);
      REQUIRE(queriedFederate.isValid());
      REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
      REQUIRE_NOTHROW(rti->requestInteractionTransportationTypeChange(
          interactionClass, transportation));
      REQUIRE_NOTHROW(rti->evokeCallback(0.0));
      REQUIRE(federate.changeCount == 1U);
      REQUIRE(federate.changedInteractionClass == interactionClass);
      REQUIRE(federate.changedTransportationType == transportation);
      REQUIRE_NOTHROW(rti->queryInteractionTransportationType(
          queriedFederate, interactionClass));
      REQUIRE_NOTHROW(rti->evokeCallback(0.0));
      REQUIRE(federate.queryCount == 1U);
      REQUIRE(federate.queriedFederate == queriedFederate);
      REQUIRE(federate.queriedInteractionClass == interactionClass);
      REQUIRE(federate.queriedTransportationType == transportation);
      rti->resignFederationExecution(NO_ACTION);
      clientJoined = false;
      rti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      if (clientJoined) {
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
    REQUIRE(expectedInteractionClass.load(std::memory_order_acquire) != 0U);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}

class DirectedInteractionTransportationQueryFederateAmbassador final
    : public NullFederateAmbassador {
 public:
  void confirmInteractionTransportationTypeChange(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::TransportationTypeHandle const& transportationType) override {
    ++changeCount;
    changedInteractionClass = interactionClass;
    changedTransportationType = transportationType;
  }

  void reportInteractionTransportationType(
      rti1516_2025::FederateHandle const& federate,
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::TransportationTypeHandle const& transportationType) override {
    ++queryCount;
    queriedFederate = federate;
    queriedInteractionClass = interactionClass;
    queriedTransportationType = transportationType;
  }

  std::size_t changeCount = 0U;
  rti1516_2025::InteractionClassHandle changedInteractionClass;
  rti1516_2025::TransportationTypeHandle changedTransportationType;
  std::size_t queryCount = 0U;
  rti1516_2025::FederateHandle queriedFederate;
  rti1516_2025::InteractionClassHandle queriedInteractionClass;
  rti1516_2025::TransportationTypeHandle queriedTransportationType;
};

TEST_CASE(
    "RTIambassador reports a directed interaction transportation override through a configured process endpoint",
    "[integration][foundation][interaction-management][object-management][transportation][transport]"
    "[directed-interaction][directed-routing][process-boundary][public-endpoint]"
    "[process-directed-interaction-transportation-query][callbacks][callback-immediate][2025]"
    "[rti.service.publish-object-class-directed-interactions][rti.service.request-interaction-transportation-type-change]"
    "[rti.service.query-interaction-transportation-type]"
    "[federate.callback.confirm-interaction-transportation-type-change]"
    "[federate.callback.report-interaction-transportation-type]") {
  auto runScenario = [](CallbackModel callbackModel) {
    constexpr wchar_t const* federationName =
        L"public-process-directed-transportation-query-execution";
    constexpr wchar_t const* federateName =
        L"public-process-directed-transportation-query-federate";
    constexpr char const* objectClassName =
        "HLAobjectRoot.UmbraDirectedFixtureObject";
    constexpr char const* interactionClassName =
        "HLAinteractionRoot.UmbraDirectedFixtureInteraction";
    constexpr char const* transportationTypeName = "HLAbestEffort";

    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    std::atomic_uint64_t expectedObjectClass{0U};
    std::atomic_uint64_t expectedInteractionClass{0U};
    std::atomic_uint64_t expectedTransportation{0U};
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry,
            composedDirectedProcessDefinition(),
            ProcessFederationServiceOptions{});
        auto connection = listener->accept(
            nullptr,
            {"public-process-directed-transportation-query-server", 0x9741U},
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
                          std::string(description) + " expected operation " +
                          std::to_string(static_cast<unsigned>(operation)) +
                          " but received " +
                          std::to_string(static_cast<unsigned>(request.operation)));
                    }
                    auto response = handler(request);
                    if (response.status != TransportServiceStatus::ok) {
                      throw std::runtime_error(
                          std::string(description) + " handler status " +
                          std::to_string(static_cast<unsigned>(response.status)));
                    }
                    return response;
                  })) {
            throw std::runtime_error(description);
          }
        };

        serveExpected(
            TransportServiceOperation::create_federation_execution,
            "The directed transportation query server lost Create.");
        auto const objectClass = registry.objectClassHandleFor(
            federationName, objectClassName);
        auto const interactionClass = registry.interactionClassHandleFor(
            federationName, interactionClassName);
        auto const transportation = registry.transportationTypeHandleFor(
            federationName, transportationTypeName);
        if (!objectClass || !interactionClass || !transportation) {
          throw std::runtime_error(
              "The directed transportation query server could not resolve its FOM handles.");
        }
        expectedObjectClass.store(*objectClass, std::memory_order_release);
        expectedInteractionClass.store(*interactionClass, std::memory_order_release);
        expectedTransportation.store(*transportation, std::memory_order_release);

        serveExpected(
            TransportServiceOperation::join_federation_execution,
            "The directed transportation query server lost Join.");
        serveExpected(
            TransportServiceOperation::get_object_class_handle,
            "The directed transportation query server lost object lookup.");
        serveExpected(
            TransportServiceOperation::get_interaction_class_handle,
            "The directed transportation query server lost interaction lookup.");
        serveExpected(
            TransportServiceOperation::get_transportation_type_handle,
            "The directed transportation query server lost transportation lookup.");
        serveExpected(
            TransportServiceOperation::get_federate_handle,
            "The directed transportation query server lost federate lookup.");
        serveExpected(
            TransportServiceOperation::publish_object_class_directed_interactions,
            "The directed transportation query server lost directed publication.");
        serveExpected(
            TransportServiceOperation::request_interaction_transportation_type_change,
            "The directed transportation query server lost directed transportation change.");
        serveExpected(
            TransportServiceOperation::receive_interaction,
            "The directed transportation query server lost transportation confirmation.");
        serveExpected(
            TransportServiceOperation::query_interaction_transportation_type,
            "The directed transportation query server lost transportation query.");
        serveExpected(
            TransportServiceOperation::receive_interaction,
            "The directed transportation query server lost transportation report.");
        serveExpected(
            TransportServiceOperation::resign_federation_execution,
            "The directed transportation query server lost Resign.");
        service.detach(session);
        connection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    DirectedInteractionTransportationQueryFederateAmbassador federate;
    auto rti = makeRti();
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-directed-transportation-query-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool clientJoined = false;
    try {
      REQUIRE(rti->connect(federate, callbackModel, configuration).addressUsed);
      rti->createFederationExecution(
          federationName, L"server-owned-directed-transportation-fom.xml");
      static_cast<void>(rti->joinFederationExecution(
          federateName,
          L"public-process-directed-transportation-query-type",
          federationName));
      clientJoined = true;

      auto const objectClass = rti->getObjectClassHandle(
          L"HLAobjectRoot.UmbraDirectedFixtureObject");
      auto const interactionClass = rti->getInteractionClassHandle(
          L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");
      auto const transportation = rti->getTransportationTypeHandle(
          L"HLAbestEffort");
      REQUIRE(objectClass.toString() ==
              L"ObjectClassHandle(" +
                  std::to_wstring(expectedObjectClass.load(
                      std::memory_order_acquire)) +
                  L")");
      REQUIRE(interactionClass.toString() ==
              L"InteractionClassHandle(" +
                  std::to_wstring(expectedInteractionClass.load(
                      std::memory_order_acquire)) +
                  L")");
      REQUIRE(transportation.toString() ==
              L"TransportationTypeHandle(" +
                  std::to_wstring(expectedTransportation.load(
                      std::memory_order_acquire)) +
                  L")");
      auto const queriedFederate = rti->getFederateHandle(federateName);
      REQUIRE(queriedFederate.isValid());
      REQUIRE_NOTHROW(rti->publishObjectClassDirectedInteractions(
          objectClass, rti1516_2025::InteractionClassHandleSet{interactionClass}));
      REQUIRE_NOTHROW(rti->requestInteractionTransportationTypeChange(
          interactionClass, transportation));
      REQUIRE_NOTHROW(rti->evokeCallback(0.0));
      REQUIRE(federate.changeCount == 1U);
      REQUIRE(federate.changedInteractionClass == interactionClass);
      REQUIRE(federate.changedTransportationType == transportation);

      REQUIRE_NOTHROW(rti->queryInteractionTransportationType(
          queriedFederate, interactionClass));
      REQUIRE_NOTHROW(rti->evokeCallback(0.0));
      REQUIRE(federate.queryCount == 1U);
      REQUIRE(federate.queriedFederate == queriedFederate);
      REQUIRE(federate.queriedInteractionClass == interactionClass);
      REQUIRE(federate.queriedTransportationType == transportation);
      rti->resignFederationExecution(NO_ACTION);
      clientJoined = false;
      rti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      if (clientJoined) {
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
    listener.reset();
    if (server.joinable()) {
      server.join();
    }
    if (serverError) {
      try {
        std::rethrow_exception(serverError);
      } catch (std::exception const& error) {
        FAIL_CHECK(std::string("directed transportation query server: ") +
                   error.what());
      } catch (...) {
        FAIL_CHECK(
            "directed transportation query server failed with an unknown exception");
      }
    }
    if (clientError) {
      std::rethrow_exception(clientError);
    }
    REQUIRE_FALSE(serverError);
    REQUIRE_FALSE(clientJoined);
    REQUIRE(expectedObjectClass.load(std::memory_order_acquire) != 0U);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}

class RegionalInteractionTransportationFederateAmbassador final
    : public NullFederateAmbassador {
 public:
  void confirmInteractionTransportationTypeChange(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::TransportationTypeHandle const& transportationType) override {
    ++changeCount;
    changedInteractionClass = interactionClass;
    changedTransportationType = transportationType;
  }

  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::ParameterHandleValueMap const& parameterValues,
      rti1516_2025::VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const& transportationType,
      rti1516_2025::FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
    ++receiveCount;
    receivedInteractionClass = interactionClass;
    receivedTransportationType = transportationType;
    receivedProducingFederate = producingFederate;
    receivedParameterCount = parameterValues.size();
    receivedRegionCount = optionalSentRegions == nullptr
        ? 0U
        : optionalSentRegions->size();
    receivedOptionalRegions = optionalSentRegions != nullptr;
    receivedTag.clear();
    if (userSuppliedTag.size() != 0U) {
      auto const* first = static_cast<std::uint8_t const*>(userSuppliedTag.data());
      receivedTag.assign(first, first + userSuppliedTag.size());
    }
  }

  std::size_t changeCount = 0U;
  rti1516_2025::InteractionClassHandle changedInteractionClass;
  rti1516_2025::TransportationTypeHandle changedTransportationType;
  std::size_t receiveCount = 0U;
  rti1516_2025::InteractionClassHandle receivedInteractionClass;
  rti1516_2025::TransportationTypeHandle receivedTransportationType;
  rti1516_2025::FederateHandle receivedProducingFederate;
  std::size_t receivedParameterCount = 0U;
  std::size_t receivedRegionCount = 0U;
  bool receivedOptionalRegions = false;
  std::vector<std::uint8_t> receivedTag;
};

TEST_CASE(
    "RTIambassadors preserve a regional interaction transportation override through a configured process endpoint",
    "[integration][foundation][data-distribution-management][interaction-management][transportation][transport][process-boundary][public-endpoint][regional-interaction][process-transportation-regional-interaction-control][rti.service.request-interaction-transportation-type-change][rti.service.subscribe-interaction-class-with-regions][rti.service.send-interaction-with-regions][federate.callback.confirm-interaction-transportation-type-change][federate.callback.receive-interaction][2025]") {
  auto runScenario = [](CallbackModel callbackModel) {
    constexpr wchar_t const* federationName =
        L"public-process-transportation-regional-interaction-execution";
    constexpr wchar_t const* senderName =
        L"public-process-transportation-regional-interaction-sender";
    constexpr wchar_t const* receiverName =
        L"public-process-transportation-regional-interaction-receiver";
    constexpr wchar_t const* federateType =
        L"public-process-transportation-regional-interaction-type";
    constexpr wchar_t const* interactionNameWide =
        L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
    constexpr wchar_t const* parameterNameWide = L"TimelinessOk";
    constexpr wchar_t const* dimensionNameWide = L"ServerId";

    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    std::atomic_uint64_t expectedInteractionClass{0U};
    std::atomic_uint64_t expectedParameter{0U};
    std::atomic_uint64_t expectedDimension{0U};
    std::atomic_uint64_t expectedTransportation{0U};
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
        auto senderConnection = listener->accept(
            nullptr,
            {"public-process-transportation-regional-interaction-server", 0x9605U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession sender(senderConnection);
        auto senderHandler = service.handlerFor(sender);
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
                    auto response = handler(request);
                    if (response.status != TransportServiceStatus::ok) {
                      throw std::runtime_error(description);
                    }
                    return response;
                  })) {
            throw std::runtime_error(description);
          }
        };

        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::create_federation_execution,
            "The regional transportation process server lost Create.");
        auto const interactionClass = registry.interactionClassHandleFor(
            federationName,
            "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
        auto const parameter = registry.parameterHandleFor(
            federationName,
            "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed",
            "TimelinessOk");
        auto const dimension = registry.dimensionHandleFor(
            federationName, "ServerId");
        auto const transportation = registry.transportationTypeHandleFor(
            federationName, "HLAbestEffort");
        if (!interactionClass || !parameter || !dimension || !transportation) {
          throw std::runtime_error(
              "The regional transportation process server could not resolve its FOM handles.");
        }
        expectedInteractionClass.store(*interactionClass, std::memory_order_release);
        expectedParameter.store(*parameter, std::memory_order_release);
        expectedDimension.store(*dimension, std::memory_order_release);
        expectedTransportation.store(*transportation, std::memory_order_release);
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::join_federation_execution,
            "The regional transportation process server lost sender Join.");

        auto receiverConnection = listener->accept(
            nullptr,
            {"public-process-transportation-regional-interaction-server", 0x9606U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession receiver(receiverConnection);
        auto receiverHandler = service.handlerFor(receiver);
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::join_federation_execution,
            "The regional transportation process server lost receiver Join.");

        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_interaction_class_handle,
            "The regional transportation process server lost sender class lookup.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_interaction_class_handle,
            "The regional transportation process server lost receiver class lookup.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_parameter_handle,
            "The regional transportation process server lost sender parameter lookup.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_parameter_handle,
            "The regional transportation process server lost receiver parameter lookup.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_dimension_handle,
            "The regional transportation process server lost sender dimension lookup.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_dimension_handle,
            "The regional transportation process server lost receiver dimension lookup.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_transportation_type_handle,
            "The regional transportation process server lost sender transportation lookup.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::create_region,
            "The regional transportation process server lost sender region creation.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::create_region,
            "The regional transportation process server lost receiver region creation.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::set_range_bounds,
            "The regional transportation process server lost sender bounds.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::commit_region_modifications,
            "The regional transportation process server lost sender region commit.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::set_range_bounds,
            "The regional transportation process server lost receiver bounds.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::commit_region_modifications,
            "The regional transportation process server lost receiver region commit.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::publish_interaction_class,
            "The regional transportation process server lost Publish.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::request_interaction_transportation_type_change,
            "The regional transportation process server lost transportation override.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::receive_interaction,
            "The regional transportation process server lost transportation confirmation.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::set_convey_region_designator_sets_switch,
            "The regional transportation process server lost Convey switch set.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_convey_region_designator_sets_switch,
            "The regional transportation process server lost Convey switch query.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::subscribe_interaction_class_with_regions,
            "The regional transportation process server lost regional Subscribe.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::send_interaction_with_regions,
            "The regional transportation process server lost regional Send.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::receive_interaction,
            "The regional transportation process server lost regional Receive.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::unsubscribe_interaction_class_with_regions,
            "The regional transportation process server lost regional Unsubscribe.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::resign_federation_execution,
            "The regional transportation process server lost sender Resign.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::resign_federation_execution,
            "The regional transportation process server lost receiver Resign.");
        service.detach(sender);
        service.detach(receiver);
        senderConnection->close();
        receiverConnection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    RegionalInteractionTransportationFederateAmbassador senderFederate;
    RegionalInteractionTransportationFederateAmbassador receiverFederate;
    auto senderRti = makeRti();
    auto receiverRti = makeRti();
    auto configurationFor = [&](wchar_t const* name) {
      return RtiConfiguration::createConfiguration()
          .withConfigurationName(name)
          .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
    };
    std::exception_ptr clientError;
    bool senderJoined = false;
    bool receiverJoined = false;
    try {
      REQUIRE(senderRti->connect(
                  senderFederate,
                  callbackModel,
                  configurationFor(L"public-process-transportation-regional-interaction-sender-client"))
                  .addressUsed);
      senderRti->createFederationExecution(
          federationName, L"server-owned-fom.xml");
      auto const senderHandle = senderRti->joinFederationExecution(
          senderName, federateType, federationName);
      REQUIRE(senderHandle.isValid());
      senderJoined = true;

      REQUIRE(receiverRti->connect(
                  receiverFederate,
                  callbackModel,
                  configurationFor(L"public-process-transportation-regional-interaction-receiver-client"))
                  .addressUsed);
      auto const receiverHandle = receiverRti->joinFederationExecution(
          receiverName, federateType, federationName);
      REQUIRE(receiverHandle.isValid());
      receiverJoined = true;

      auto const senderInteraction =
          senderRti->getInteractionClassHandle(interactionNameWide);
      auto const receiverInteraction =
          receiverRti->getInteractionClassHandle(interactionNameWide);
      auto const expectedClass =
          rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
              expectedInteractionClass.load(std::memory_order_acquire));
      REQUIRE(senderInteraction.toString() == expectedClass.toString());
      REQUIRE(receiverInteraction.toString() == expectedClass.toString());
      auto const senderParameter =
          senderRti->getParameterHandle(senderInteraction, parameterNameWide);
      auto const receiverParameter =
          receiverRti->getParameterHandle(receiverInteraction, parameterNameWide);
      auto const expectedParameterHandle =
          rti1516_2025::umbra_binding_detail::makeParameterHandle(
              expectedParameter.load(std::memory_order_acquire));
      REQUIRE(senderParameter == expectedParameterHandle);
      REQUIRE(receiverParameter == expectedParameterHandle);
      auto const senderDimension =
          senderRti->getDimensionHandle(dimensionNameWide);
      auto const receiverDimension =
          receiverRti->getDimensionHandle(dimensionNameWide);
      auto const expectedDimensionText =
          L"DimensionHandle(" +
          std::to_wstring(expectedDimension.load(std::memory_order_acquire)) +
          L")";
      REQUIRE(senderDimension.toString() == expectedDimensionText);
      REQUIRE(receiverDimension.toString() == expectedDimensionText);
      auto const transportation =
          senderRti->getTransportationTypeHandle(L"HLAbestEffort");
      auto const expectedTransportationText =
          L"TransportationTypeHandle(" +
          std::to_wstring(expectedTransportation.load(std::memory_order_acquire)) +
          L")";
      REQUIRE(transportation.toString() == expectedTransportationText);

      auto const senderRegion = senderRti->createRegion(
          rti1516_2025::DimensionHandleSet{senderDimension});
      auto const receiverRegion = receiverRti->createRegion(
          rti1516_2025::DimensionHandleSet{receiverDimension});
      REQUIRE(senderRegion.isValid());
      REQUIRE(receiverRegion.isValid());
      REQUIRE_NOTHROW(senderRti->setRangeBounds(
          senderRegion, senderDimension, rti1516_2025::RangeBounds(0UL, 5UL)));
      REQUIRE_NOTHROW(senderRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{senderRegion}));
      REQUIRE_NOTHROW(receiverRti->setRangeBounds(
          receiverRegion,
          receiverDimension,
          rti1516_2025::RangeBounds(0UL, 5UL)));
      REQUIRE_NOTHROW(receiverRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{receiverRegion}));

      REQUIRE_NOTHROW(senderRti->publishInteractionClass(senderInteraction));
      REQUIRE_NOTHROW(senderRti->requestInteractionTransportationTypeChange(
          senderInteraction, transportation));
      REQUIRE_NOTHROW(senderRti->evokeCallback(0.0));
      REQUIRE(senderFederate.changeCount == 1U);
      REQUIRE(senderFederate.changedInteractionClass == senderInteraction);
      REQUIRE(senderFederate.changedTransportationType == transportation);

      REQUIRE_NOTHROW(receiverRti->setConveyRegionDesignatorSetsSwitch(true));
      REQUIRE(receiverRti->getConveyRegionDesignatorSetsSwitch());
      REQUIRE_NOTHROW(receiverRti->subscribeInteractionClassWithRegions(
          receiverInteraction,
          rti1516_2025::RegionHandleSet{receiverRegion},
          true));
      ParameterHandleValueMap parameterValues;
      std::array<std::uint8_t, 2U> encodedParameter{0x01U, 0x00U};
      parameterValues.emplace(
          senderParameter,
          VariableLengthData(encodedParameter.data(), encodedParameter.size()));
      std::array<std::uint8_t, 1U> tag{0xB5U};
      REQUIRE_NOTHROW(senderRti->sendInteractionWithRegions(
          senderInteraction,
          parameterValues,
          rti1516_2025::RegionHandleSet{senderRegion},
          VariableLengthData(tag.data(), tag.size())));
      REQUIRE_NOTHROW(receiverRti->evokeCallback(0.0));

      REQUIRE(receiverFederate.receiveCount == 1U);
      REQUIRE(receiverFederate.receivedInteractionClass == receiverInteraction);
      REQUIRE(receiverFederate.receivedTransportationType == transportation);
      REQUIRE(receiverFederate.receivedProducingFederate == senderHandle);
      REQUIRE(receiverFederate.receivedParameterCount == 1U);
      REQUIRE(receiverFederate.receivedOptionalRegions);
      REQUIRE(receiverFederate.receivedRegionCount == 1U);
      REQUIRE(receiverFederate.receivedTag == std::vector<std::uint8_t>{0xB5U});

      REQUIRE_NOTHROW(receiverRti->unsubscribeInteractionClassWithRegions(
          receiverInteraction,
          rti1516_2025::RegionHandleSet{receiverRegion}));
      senderRti->resignFederationExecution(NO_ACTION);
      senderJoined = false;
      receiverRti->resignFederationExecution(NO_ACTION);
      receiverJoined = false;
      senderRti->disconnect();
      receiverRti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      if (senderJoined) {
        try {
          senderRti->resignFederationExecution(NO_ACTION);
        } catch (...) {
        }
      }
      if (receiverJoined) {
        try {
          receiverRti->resignFederationExecution(NO_ACTION);
        } catch (...) {
        }
      }
      try {
        senderRti->disconnect();
      } catch (...) {
      }
      try {
        receiverRti->disconnect();
      } catch (...) {
      }
    }
    listener.reset();
    if (server.joinable()) {
      server.join();
    }
    if (clientError) {
      if (serverError) {
        std::rethrow_exception(serverError);
      }
      std::rethrow_exception(clientError);
    }
    REQUIRE_FALSE(serverError);
    REQUIRE_FALSE(senderJoined);
    REQUIRE_FALSE(receiverJoined);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}

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
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::get_dimension_upper_bound) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_send_service_reports_to_file_switch) ||
          !serveExpected(TransportServiceOperation::get_dimension_upper_bound) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_send_service_reports_to_file_switch) ||
          !serveExpected(TransportServiceOperation::get_dimension_upper_bound) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_send_service_reports_to_file_switch) ||
          !serveExpected(TransportServiceOperation::get_dimension_upper_bound) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_send_service_reports_to_file_switch) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_object_class_relevance_advisory_switch) ||
          !serveExpected(TransportServiceOperation::get_attribute_relevance_advisory_switch) ||
          !serveExpected(TransportServiceOperation::get_attribute_scope_advisory_switch) ||
          !serveExpected(TransportServiceOperation::get_interaction_relevance_advisory_switch) ||
          !serveExpected(TransportServiceOperation::get_convey_region_designator_sets_switch) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
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

TEST_CASE(
    "RTIambassador consumes process federation-wide HLAsetSwitches Auto Provide",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[process-mom-federation-set-switches][process-boundary][public-endpoint][2025]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-interaction-class-handle]"
    "[rti.service.get-parameter-handle][rti.service.send-interaction]"
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

  constexpr wchar_t const* federationName =
      L"public-process-federation-mom-switches-execution";
  constexpr wchar_t const* federateName =
      L"public-process-federation-mom-switches-federate";
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto connection = listener->accept(
          nullptr,
          {"public-process-federation-mom-switches-server", 0x9E02U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return umbra::test::servePrimaryProcessRequest(
            session, handler,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The process federation MOM set-switches server received an unexpected operation.");
              }
              return handler(request);
            });
      };
      auto requireAutoProvide = [&](bool expected) {
        auto const member = registry.memberByName(federationName, federateName);
        auto const value = member
            ? registry.autoProvideSwitchFor(federationName, member->id)
            : std::optional<bool>{};
        if (!value || *value != expected) {
          throw std::runtime_error(
              "The process federation did not apply the expected federation-wide HLAsetSwitches value.");
        }
      };
      if (!serveExpected(TransportServiceOperation::create_federation_execution) ||
          !serveExpected(TransportServiceOperation::join_federation_execution)) {
        throw std::runtime_error(
            "The process federation MOM set-switches server did not receive create and join.");
      }
      requireAutoProvide(true);
      if (!serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::send_interaction)) {
        throw std::runtime_error(
            "The process federation MOM set-switches server did not receive the disabling adjustment.");
      }
      requireAutoProvide(false);
      if (!serveExpected(TransportServiceOperation::send_interaction)) {
        throw std::runtime_error(
            "The process federation MOM set-switches server did not receive the enabling adjustment.");
      }
      requireAutoProvide(true);
      if (!serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The process federation MOM set-switches server did not receive resignation.");
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
                               L"public-process-federation-mom-switches-client")
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
        federateName, L"public-process-federation-mom-switches-type", federationName));
    joined = true;

    auto const setSwitches = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches");
    auto const autoProvide = rti->getParameterHandle(setSwitches, L"HLAautoProvide");
    REQUIRE(setSwitches.isValid());
    REQUIRE(autoProvide.isValid());

    auto encodeSwitch = [](bool const enabled) {
      return rti1516_2025::HLAinteger32BE(enabled ? 1 : 0).encode();
    };
    ParameterHandleValueMap const disabledValues{
        {autoProvide, encodeSwitch(false)}};
    REQUIRE_NOTHROW(rti->sendInteraction(
        setSwitches, disabledValues, VariableLengthData()));

    ParameterHandleValueMap const enabledValues{
        {autoProvide, encodeSwitch(true)}};
    REQUIRE_NOTHROW(rti->sendInteraction(
        setSwitches, enabledValues, VariableLengthData()));

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
  REQUIRE(connectionResult.has_value());
}

TEST_CASE(
    "RTIambassador processes predefined process HLAsetSwitches parameters through a compatible subclass and ignores extensions",
    "[integration][development-profile][foundation][federation-management][mom][switches]"
    "[mom-extension][process-mom-extension-parameter][extension-subclass][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-interaction-class-handle]"
    "[rti.service.get-parameter-handle][rti.service.get-service-reporting-switch]"
    "[rti.service.get-automatic-resign-directive][rti.service.send-interaction]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::FomCompositionStatus;
  using umbra::detail::FomModuleKind;
  using umbra::detail::LibXml2FomModuleComposer;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportServiceDispatcher;
  using umbra::detail::ProcessTransportSession;
  using umbra::detail::TransportServiceMessage;
  using umbra::detail::TransportServiceOperation;

  auto composeExtensionDefinition = [] {
    std::vector<PrevalidatedFomModule> modules{
        validatedProcessModule(
            processResourcePath("mim/HLAstandardMIM-2025.xml"),
            FomModuleKind::mim,
            L"urn:umbra:test:process-mom-extension-mim"),
        validatedProcessModule(
            processResourcePath("examples/RestaurantFOMmodule-2025.xml"),
            FomModuleKind::fom,
            L"urn:umbra:test:process-mom-extension-restaurant"),
        validatedProcessModule(
            std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
                "data" / "mom-set-switches-extension-fom.xml",
            FomModuleKind::fom,
            L"urn:umbra:test:process-mom-extension-fom"),
    };
    LibXml2FomModuleComposer composer(
        processResourcePath("schemas/IEEE1516-FDD-2025.xsd"));
    auto result = composer.compose(modules);
    if (result.status != FomCompositionStatus::valid || !result.catalog ||
        !result.fdd) {
      throw std::runtime_error(
          "The process MOM extension FOM did not compose.");
    }
    return FederationDefinition{
        std::move(result.modules),
        L"HLAinteger64Time",
        std::move(result.catalog),
        std::move(result.fdd),
    };
  };

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-mom-extension-parameter-execution";
  constexpr wchar_t const* federateName =
      L"public-process-mom-extension-parameter-federate";
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composeExtensionDefinition(), ProcessFederationServiceOptions{});
      auto connection = listener->accept(
          nullptr,
          {"public-process-mom-extension-parameter-server", 0x9E21U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return umbra::test::servePrimaryProcessRequest(
            session, handler,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The process MOM extension server received an unexpected operation.");
              }
              return handler(request);
            });
      };
      if (!serveExpected(TransportServiceOperation::create_federation_execution) ||
          !serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::get_automatic_resign_directive) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::get_automatic_resign_directive) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::get_automatic_resign_directive) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_automatic_resign_directive) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_automatic_resign_directive) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The process MOM extension server did not receive the complete sequence.");
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
                               L"public-process-mom-extension-parameter-client")
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
        federateName, L"public-process-mom-extension-parameter-type", federationName));
    joined = true;

    auto const setSwitches = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
    auto const extendedSetSwitches = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches.UmbraExtendedSetSwitches");
    REQUIRE(setSwitches.isValid());
    REQUIRE(extendedSetSwitches.isValid());
    auto const extensionPayload = rti->getParameterHandle(
        setSwitches, L"UmbraExtensionSwitchPayload");
    auto const subclassPayload = rti->getParameterHandle(
        extendedSetSwitches, L"UmbraExtendedSwitchPayload");
    REQUIRE(extensionPayload.isValid());
    REQUIRE(subclassPayload.isValid());
    auto const serviceReporting = rti->getParameterHandle(
        extendedSetSwitches, L"HLAserviceReporting");
    REQUIRE(serviceReporting.isValid());
    auto const automaticResign = rti->getParameterHandle(
        extendedSetSwitches, L"HLAautomaticResignAction");
    REQUIRE(automaticResign.isValid());

    auto const initialServiceReporting = rti->getServiceReportingSwitch();
    auto const initialResignAction = rti->getAutomaticResignDirective();
    REQUIRE_THROWS_AS(
        rti->sendInteraction(
            setSwitches,
            ParameterHandleValueMap{
                {extensionPayload, VariableLengthData("extension", 9U)},
            },
            VariableLengthData()),
        RTIinternalError);
    REQUIRE(rti->getServiceReportingSwitch() == initialServiceReporting);
    REQUIRE(rti->getAutomaticResignDirective() == initialResignAction);

    REQUIRE_THROWS_AS(
        rti->sendInteraction(
            extendedSetSwitches,
            ParameterHandleValueMap{
                {subclassPayload, VariableLengthData("subclass", 8U)},
            },
            VariableLengthData()),
        RTIinternalError);
    REQUIRE(rti->getServiceReportingSwitch() == initialServiceReporting);
    REQUIRE(rti->getAutomaticResignDirective() == initialResignAction);

    auto encodeSwitch = [](bool const enabled) {
      return rti1516_2025::HLAinteger32BE(enabled ? 1 : 0).encode();
    };
    auto const promotedServiceReporting = !initialServiceReporting;
    auto encodeResignAction = [](rti1516_2025::ResignAction const action) {
      return rti1516_2025::HLAinteger32BE(
                 static_cast<std::int32_t>(action))
          .encode();
    };
    REQUIRE_NOTHROW(rti->sendInteraction(
        extendedSetSwitches,
        ParameterHandleValueMap{
            {serviceReporting, encodeSwitch(promotedServiceReporting)},
            {subclassPayload, VariableLengthData("subclass", 8U)},
        },
        VariableLengthData()));
    REQUIRE(rti->getServiceReportingSwitch() == promotedServiceReporting);
    REQUIRE_NOTHROW(rti->sendInteraction(
        extendedSetSwitches,
        ParameterHandleValueMap{
            {serviceReporting, encodeSwitch(initialServiceReporting)},
        },
        VariableLengthData()));
    REQUIRE(rti->getServiceReportingSwitch() == initialServiceReporting);

    auto const promotedResignAction = encodeResignAction(DELETE_OBJECTS);
    REQUIRE_NOTHROW(rti->sendInteraction(
        extendedSetSwitches,
        ParameterHandleValueMap{
            {automaticResign, promotedResignAction},
            {subclassPayload, VariableLengthData("subclass", 8U)},
        },
        VariableLengthData()));
    REQUIRE(rti->getAutomaticResignDirective() == DELETE_OBJECTS);
    REQUIRE_NOTHROW(rti->sendInteraction(
        extendedSetSwitches,
        ParameterHandleValueMap{
            {automaticResign, encodeResignAction(initialResignAction)},
        },
        VariableLengthData()));
    REQUIRE(rti->getAutomaticResignDirective() == initialResignAction);

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
  REQUIRE(connectionResult.has_value());
}
#endif

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassador preserves the process HLAsetSwitches Service Reporting interlock",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[switches][service-reporting][mom-service-reporting-interlock]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-interaction-class-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.get-service-reporting-switch]"
    "[rti.service.get-parameter-handle][rti.service.send-interaction]"
    "[rti.service.unsubscribe-interaction-class]"
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

  constexpr wchar_t const* federationName =
      L"public-process-mom-service-report-interlock-execution";
  constexpr wchar_t const* federateName =
      L"public-process-mom-service-report-interlock-federate";
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto connection = listener->accept(
          nullptr,
          {"public-process-mom-service-report-interlock-server", 0x9E02U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return umbra::test::servePrimaryProcessRequest(
            session, handler,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The process MOM service-report interlock server expected operation " +
                    std::to_string(static_cast<unsigned>(operation)) +
                    " but received " +
                    std::to_string(static_cast<unsigned>(request.operation)) + ".");
              }
              return handler(request);
            });
      };
      if (!serveExpected(TransportServiceOperation::create_federation_execution) ||
          !serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::subscribe_interaction_class) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::unsubscribe_interaction_class) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::subscribe_interaction_class) ||
          !serveExpected(TransportServiceOperation::report_failed_service_invocation) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The process MOM service-report interlock server did not receive the complete sequence.");
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
                               L"public-process-mom-service-report-interlock-client")
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
        federateName,
        L"public-process-mom-service-report-interlock-type",
        federationName));
    joined = true;

    auto const reportInvocation = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
    REQUIRE(reportInvocation.isValid());
    REQUIRE_NOTHROW(rti->subscribeInteractionClass(reportInvocation, true));
    REQUIRE_FALSE(rti->getServiceReportingSwitch());

    auto const setSwitches = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
    auto const serviceReporting = rti->getParameterHandle(
        setSwitches, L"HLAserviceReporting");
    REQUIRE(setSwitches.isValid());
    REQUIRE(serviceReporting.isValid());

    auto const encodedSwitch =
        rti1516_2025::HLAinteger32BE(1).encode();
    ParameterHandleValueMap const enabledValues{
        {serviceReporting, encodedSwitch}};
    REQUIRE_THROWS_AS(
        rti->sendInteraction(
            setSwitches, enabledValues, VariableLengthData()),
        RTIinternalError);
    REQUIRE_FALSE(rti->getServiceReportingSwitch());

    REQUIRE_NOTHROW(rti->unsubscribeInteractionClass(reportInvocation));
    REQUIRE_NOTHROW(rti->sendInteraction(
        setSwitches, enabledValues, VariableLengthData()));
    REQUIRE(rti->getServiceReportingSwitch());

    REQUIRE_THROWS_AS(
        rti->subscribeInteractionClass(reportInvocation, true),
        rti1516_2025::FederateServiceInvocationsAreBeingReportedViaMOM);
    REQUIRE(rti->getServiceReportingSwitch());

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
  REQUIRE(connectionResult.has_value());
}

TEST_CASE(
    "RTIambassador reports a rejected process HLAsetSwitches interaction through HLAreportMOMexception",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[switches][service-reporting][mom-exception][process-mom-exception-report]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-interaction-class-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.get-service-reporting-switch]"
    "[rti.service.get-parameter-handle][rti.service.send-interaction]"
    "[rti.service.unsubscribe-interaction-class]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]"
    "[federate.callback.receive-interaction]") {
  class MomExceptionFederateAmbassador final : public NullFederateAmbassador {
   public:
    void receiveInteraction(
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
      ++receivedInteractionCount;
      receivedInteractionClass = interactionClass;
      receivedParameterValues = parameterValues;
      receivedTagSize = userSuppliedTag.size();
      receivedTransportationType = transportationType;
      receivedProducingFederate = producingFederate;
      receivedSentRegions = optionalSentRegions != nullptr;
    }

    std::size_t receivedInteractionCount = 0U;
    rti1516_2025::InteractionClassHandle receivedInteractionClass;
    rti1516_2025::ParameterHandleValueMap receivedParameterValues;
    std::size_t receivedTagSize = 0U;
    rti1516_2025::TransportationTypeHandle receivedTransportationType;
    rti1516_2025::FederateHandle receivedProducingFederate;
    bool receivedSentRegions = false;
  };

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

  constexpr wchar_t const* federationName =
      L"public-process-mom-exception-report-execution";
  constexpr wchar_t const* federateName =
      L"public-process-mom-exception-report-federate";
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          composedProcessDefinition(),
          ProcessFederationServiceOptions{true});
      auto connection = listener->accept(
          nullptr,
          {"public-process-mom-exception-report-server", 0x9E03U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return umbra::test::servePrimaryProcessRequest(
            session, handler,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The process MOM exception-report server received an unexpected operation.");
              }
              return handler(request);
            });
      };
      if (!serveExpected(TransportServiceOperation::create_federation_execution) ||
          !serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::subscribe_interaction_class) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::subscribe_interaction_class) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_transportation_type_handle) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::unsubscribe_interaction_class) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The process MOM exception-report server did not receive the complete sequence.");
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  MomExceptionFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"public-process-mom-exception-report-client")
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
        federateName,
        L"public-process-mom-exception-report-type",
        federationName));
    joined = true;

    auto const reportClass = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception");
    REQUIRE(reportClass.isValid());
    auto const serviceParameter = rti->getParameterHandle(
        reportClass, L"HLAservice");
    auto const exceptionParameter = rti->getParameterHandle(
        reportClass, L"HLAexception");
    auto const parameterErrorParameter = rti->getParameterHandle(
        reportClass, L"HLAparameterError");
    REQUIRE(serviceParameter.isValid());
    REQUIRE(exceptionParameter.isValid());
    REQUIRE(parameterErrorParameter.isValid());
    REQUIRE_NOTHROW(rti->subscribeInteractionClass(reportClass, true));
    REQUIRE_FALSE(rti->getServiceReportingSwitch());

    auto const reportInvocation = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
    REQUIRE(reportInvocation.isValid());
    REQUIRE_NOTHROW(rti->subscribeInteractionClass(reportInvocation, true));

    auto const setSwitches = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
    auto const serviceReporting = rti->getParameterHandle(
        setSwitches, L"HLAserviceReporting");
    REQUIRE(setSwitches.isValid());
    REQUIRE(serviceReporting.isValid());

    auto const encodedSwitch = rti1516_2025::HLAinteger32BE(1).encode();
    REQUIRE_THROWS_AS(
        rti->sendInteraction(
            setSwitches,
            ParameterHandleValueMap{{serviceReporting, encodedSwitch}},
            VariableLengthData()),
        RTIinternalError);
    while (rti->evokeCallback(0.0)) {
    }
    REQUIRE(federate.receivedInteractionCount == 1U);
    REQUIRE(federate.receivedInteractionClass == reportClass);
    REQUIRE(federate.receivedParameterValues.size() == 4U);
    REQUIRE(federate.receivedTagSize == 0U);
    REQUIRE(federate.receivedTransportationType ==
            rti->getTransportationTypeHandle(L"HLAreliable"));
    REQUIRE_FALSE(federate.receivedProducingFederate.isValid());
    REQUIRE_FALSE(federate.receivedSentRegions);

    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(
        federate.receivedParameterValues.at(serviceParameter)));
    REQUIRE(decodedService.get() ==
            L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(
        federate.receivedParameterValues.at(exceptionParameter)));
    REQUIRE(decodedException.get().find(L"RTIinternalError") !=
            std::wstring::npos);
    rti1516_2025::HLAboolean decodedParameterError;
    REQUIRE_NOTHROW(decodedParameterError.decode(
        federate.receivedParameterValues.at(parameterErrorParameter)));
    REQUIRE_FALSE(decodedParameterError.get());
    REQUIRE_FALSE(rti->getServiceReportingSwitch());

    REQUIRE_NOTHROW(rti->unsubscribeInteractionClass(reportInvocation));
    REQUIRE_NOTHROW(rti->sendInteraction(
        setSwitches,
        ParameterHandleValueMap{{serviceReporting, encodedSwitch}},
        VariableLengthData()));
    REQUIRE(rti->getServiceReportingSwitch());

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
  REQUIRE(connectionResult.has_value());
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>
#endif


#endif

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "tests/ieee1516_2025_connection_process_service_report_interaction_helpers.hpp"

TEST_CASE(
    "RTIambassador delivers failed process dimension-upper-bound reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][data-distribution-management]"
    "[ddm][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-upper-bound-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-upper-bound]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionUpperBound,
      ProcessFailedServiceReportHandleState::BindingInvalid);
}

TEST_CASE(
    "RTIambassador delivers failed process dimension-upper-bound unknown-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][data-distribution-management]"
    "[ddm][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-upper-bound-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-upper-bound]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionUpperBound,
      ProcessFailedServiceReportHandleState::FederationUnknown);
}

TEST_CASE(
    "RTIambassador delivers failed process dimension-name unknown-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][data-distribution-management]"
    "[ddm][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-name-unknown-handle-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionName,
      ProcessFailedServiceReportHandleState::FederationUnknown);
}

TEST_CASE(
    "RTIambassador delivers failed process GetAvailableDimensionsForObjectClass reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][data-distribution-management]"
    "[ddm][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-dimensions-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForObjectClass,
      ProcessFailedServiceReportHandleState::BindingInvalid);
}

TEST_CASE(
    "RTIambassador delivers failed process GetAvailableDimensionsForInteractionClass reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][data-distribution-management]"
    "[ddm][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-interaction-dimensions-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForInteractionClass,
      ProcessFailedServiceReportHandleState::BindingInvalid);
}

TEST_CASE(
    "RTIambassador delivers failed process GetAvailableDimensionsForInteractionClass unknown-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][data-distribution-management]"
    "[ddm][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-interaction-dimensions-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForInteractionClass,
      ProcessFailedServiceReportHandleState::FederationUnknown);
}

TEST_CASE(
    "RTIambassador delivers failed process GetAvailableDimensionsForObjectClass unknown-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][data-distribution-management]"
    "[ddm][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-dimensions-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForObjectClass,
      ProcessFailedServiceReportHandleState::FederationUnknown);
}

TEST_CASE(
    "RTIambassador delivers failed process GetObjectClassHandle unknown-name reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-object-class-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-object-class-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetObjectClassHandleNameNotFound,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador delivers failed process GetObjectClassName invalid-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-object-class-name-invalid-handle-interaction]"
    "[process-service-report-object-class-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-object-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetObjectClassNameInvalidHandle,
      ProcessFailedServiceReportHandleState::FederationUnknown);
}

TEST_CASE(
    "RTIambassador delivers failed process GetObjectClassName malformed-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-object-class-name-malformed-handle-interaction]"
    "[process-service-report-object-class-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-object-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetObjectClassNameInvalidHandle,
      ProcessFailedServiceReportHandleState::BindingInvalid);
}

TEST_CASE(
    "RTIambassador delivers failed process GetInteractionClassName invalid-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-interaction-class-name-invalid-handle-interaction]"
    "[process-service-report-interaction-class-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetInteractionClassNameInvalidHandle,
      ProcessFailedServiceReportHandleState::FederationUnknown);
}

TEST_CASE(
    "RTIambassador delivers failed process GetInteractionClassName malformed-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-interaction-class-name-malformed-handle-interaction]"
    "[process-service-report-interaction-class-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetInteractionClassNameInvalidHandle,
      ProcessFailedServiceReportHandleState::BindingInvalid);
}

TEST_CASE(
    "RTIambassador delivers failed process GetInteractionClassHandle unknown-name reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-interaction-class-handle-name-not-found-interaction]"
    "[process-service-report-interaction-class-name-lookup-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-handle]"
    "[rti.service.get-parameter-handle][rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetInteractionClassHandleNameNotFound,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador delivers failed process GetTransportationTypeHandle invalid-name reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-transportation-type-handle-invalid-name-interaction]"
    "[process-service-report-transportation-type-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetTransportationTypeHandleInvalidName,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador delivers failed process GetTransportationTypeName federation-unknown-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-transportation-type-name-federation-unknown-handle-interaction]"
    "[process-service-report-transportation-type-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-transportation-type-name]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetTransportationTypeNameInvalidHandle,
      ProcessFailedServiceReportHandleState::FederationUnknown);
}

TEST_CASE(
    "RTIambassador delivers failed process GetTransportationTypeName malformed-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-transportation-type-name-malformed-handle-interaction]"
    "[process-service-report-transportation-type-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-transportation-type-name]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetTransportationTypeNameInvalidHandle,
      ProcessFailedServiceReportHandleState::BindingInvalid);
}

TEST_CASE(
    "RTIambassador delivers failed process GetOrderName invalid-enum reports through the MOM interaction",
    "[integration][foundation][time-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-order-name-invalid-enum-interaction]"
    "[process-service-report-order-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-order-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetOrderNameInvalidType,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetOrderName invalid enum",
    "[integration][development-profile][foundation][time-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][transport][process-boundary][public-endpoint]"
    "[2025][process-service-report-order-name-invalid-enum-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-order-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetOrderNameInvalidType,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetObjectClassName",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][object-management][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[process-service-report-object-class-name-invalid-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-object-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetObjectClassNameInvalidHandle,
      ProcessFailedServiceReportHandleState::FederationUnknown,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetObjectClassName with malformed handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][object-management][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[process-service-report-object-class-name-malformed-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-object-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetObjectClassNameInvalidHandle,
      ProcessFailedServiceReportHandleState::BindingInvalid,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetInteractionClassName",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][object-management][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[process-service-report-interaction-class-name-invalid-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetInteractionClassNameInvalidHandle,
      ProcessFailedServiceReportHandleState::FederationUnknown,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetInteractionClassName with malformed handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][object-management][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[process-service-report-interaction-class-name-malformed-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetInteractionClassNameInvalidHandle,
      ProcessFailedServiceReportHandleState::BindingInvalid,
      true);
}

TEST_CASE(
    "RTIambassador delivers successful process SendInteraction reports through the MOM interaction with file reporting disabled",
    "[integration][development-profile][foundation][interaction-management][mom]"
    "[service-reporting][service-report-interaction][service-success][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[process-service-report-send-interaction-success-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.publish-interaction-class][rti.service.send-interaction]"
    "[rti.service.subscribe-interaction-class][rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][federate.callback.receive-interaction]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::SendInteractionSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador delivers successful process GetOrderName reports through the MOM interaction with file reporting disabled",
    "[integration][development-profile][foundation][time-management][mom]"
    "[service-reporting][service-report-interaction][service-success][support-services]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-order-name-success-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-order-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetOrderNameSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador delivers successful process GetOrderType reports through the MOM interaction with file reporting disabled",
    "[integration][development-profile][foundation][time-management][mom]"
    "[service-reporting][service-report-interaction][service-success][support-services]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-order-type-success-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-order-type]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetOrderTypeSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful SendInteraction",
    "[integration][development-profile][foundation][interaction-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-send-interaction-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch][rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.publish-interaction-class][rti.service.send-interaction]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::SendInteractionSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador delivers successful process SendInteractionWithRegions reports through the MOM interaction with file reporting disabled",
    "[integration][development-profile][foundation][interaction-management][data-distribution-management][mom]"
    "[service-reporting][service-report-interaction][service-success][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-send-interaction-with-regions-success-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch][rti.service.set-service-reporting-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-dimension-handle][rti.service.create-region]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.publish-interaction-class][rti.service.send-interaction-with-regions]"
    "[rti.service.subscribe-interaction-class][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]"
    "[federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::SendInteractionWithRegionsSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful SendInteractionWithRegions",
    "[integration][development-profile][foundation][interaction-management][data-distribution-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction][service-success][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-send-interaction-with-regions-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch][rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-dimension-handle][rti.service.create-region]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.publish-interaction-class][rti.service.send-interaction-with-regions]"
    "[rti.service.subscribe-interaction-class][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]"
    "[federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::SendInteractionWithRegionsSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador delivers failed process SendInteractionWithRegions reports through the MOM interaction with file reporting disabled",
    "[integration][development-profile][foundation][interaction-management][data-distribution-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-send-interaction-with-regions-invalid-region-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch][rti.service.set-service-reporting-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.publish-interaction-class][rti.service.send-interaction-with-regions]"
    "[rti.service.get-transportation-type-handle][rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]"
    "[federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::
          SendInteractionWithRegionsInvalidRegion,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador records invalid RegionHandle failures from process SendInteractionWithRegions in the selected service-report file",
    "[integration][development-profile][foundation][interaction-management][data-distribution-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction][service-failure][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-send-interaction-with-regions-invalid-region-failure-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch][rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.publish-interaction-class][rti.service.send-interaction-with-regions]"
    "[rti.service.get-transportation-type-handle][rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]"
    "[federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::
          SendInteractionWithRegionsInvalidRegion,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador records InteractionParameterNotDefined failures from process SendInteractionWithRegions in the selected service-report file",
    "[integration][development-profile][foundation][interaction-management][data-distribution-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction][service-failure][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-send-interaction-with-regions-invalid-parameter-failure-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch][rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-dimension-handle][rti.service.create-region]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.publish-interaction-class][rti.service.send-interaction-with-regions]"
    "[rti.service.get-transportation-type-handle][rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]"
    "[federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::
          SendInteractionWithRegionsInvalidParameter,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador delivers InteractionParameterNotDefined failures from process SendInteractionWithRegions through the MOM interaction with file reporting disabled",
    "[integration][development-profile][foundation][interaction-management][data-distribution-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-send-interaction-with-regions-invalid-parameter-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch][rti.service.set-service-reporting-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-dimension-handle][rti.service.create-region]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.publish-interaction-class][rti.service.send-interaction-with-regions]"
    "[rti.service.get-transportation-type-handle][rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]"
    "[federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::
          SendInteractionWithRegionsInvalidParameter,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM when file reporting is enabled for GetOrderName",
    "[integration][development-profile][foundation][time-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][transport][process-boundary][public-endpoint]"
    "[2025][process-service-report-order-name-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-order-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetOrderNameSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM when file reporting is enabled for GetOrderType",
    "[integration][development-profile][foundation][time-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][transport][process-boundary][public-endpoint]"
    "[2025][process-service-report-order-type-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-order-type]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetOrderTypeSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetTransportationTypeName",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][transport][process-boundary][public-endpoint]"
    "[2025][process-service-report-transportation-type-name-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-transportation-type-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetTransportationTypeNameInvalidHandle,
      ProcessFailedServiceReportHandleState::FederationUnknown,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetTransportationTypeName with malformed handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][transport][process-boundary][public-endpoint]"
    "[2025][process-service-report-transportation-type-name-malformed-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-transportation-type-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetTransportationTypeNameInvalidHandle,
      ProcessFailedServiceReportHandleState::BindingInvalid,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetTransportationTypeHandle invalid name",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][transport][process-boundary][public-endpoint]"
    "[2025][process-service-report-transportation-type-handle-invalid-name-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetTransportationTypeHandleInvalidName,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetInteractionClassHandle unknown name",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][object-management][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[process-service-report-interaction-class-handle-name-not-found-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetInteractionClassHandleNameNotFound,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetTransportationTypeName with federation-unknown handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][transport][process-boundary][public-endpoint]"
    "[2025][process-service-report-transportation-type-name-federation-unknown-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-transportation-type-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetTransportationTypeNameInvalidHandle,
      ProcessFailedServiceReportHandleState::FederationUnknown,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful GetTransportationTypeName",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][transport][process-boundary][public-endpoint]"
    "[2025][process-service-report-transportation-type-name-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-transportation-type-name]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetTransportationTypeNameSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetDimensionUpperBound with binding-invalid handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-upper-bound-invalid-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-upper-bound]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionUpperBound,
      ProcessFailedServiceReportHandleState::BindingInvalid,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetDimensionUpperBound with federation-unknown handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-upper-bound-unknown-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-upper-bound]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionUpperBound,
      ProcessFailedServiceReportHandleState::FederationUnknown,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetDimensionName with binding-invalid handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-name-binding-invalid-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionName,
      ProcessFailedServiceReportHandleState::BindingInvalid,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetAvailableDimensionsForObjectClass with binding-invalid handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-dimensions-invalid-object-class-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForObjectClass,
      ProcessFailedServiceReportHandleState::BindingInvalid,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetAvailableDimensionsForInteractionClass with binding-invalid handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-interaction-dimensions-invalid-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForInteractionClass,
      ProcessFailedServiceReportHandleState::BindingInvalid,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetAvailableDimensionsForInteractionClass with federation-unknown handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-interaction-dimensions-unknown-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForInteractionClass,
      ProcessFailedServiceReportHandleState::FederationUnknown,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetAvailableDimensionsForObjectClass with federation-unknown handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-dimensions-unknown-object-class-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForObjectClass,
      ProcessFailedServiceReportHandleState::FederationUnknown,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful GetAvailableDimensionsForObjectClass",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-dimensions-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-object-class-handle][rti.service.get-dimension-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForObjectClass,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful GetAvailableDimensionsForInteractionClass",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-interaction-dimensions-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-dimension-handle]"
    "[rti.service.get-parameter-handle][rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForInteractionClass,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful GetDimensionName",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-name-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-name][rti.service.get-dimension-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionName,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful GetObjectClassName",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][object-management][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[process-service-report-object-class-name-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-object-class-name][rti.service.get-object-class-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetObjectClassNameSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful GetInteractionClassName",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][object-management][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[process-service-report-interaction-class-name-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetInteractionClassNameSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetDimensionName with federation-unknown handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-name-unknown-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionName,
      ProcessFailedServiceReportHandleState::FederationUnknown,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful GetDimensionUpperBound",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-upper-bound-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-upper-bound][rti.service.get-dimension-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionUpperBound,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}
#endif
#endif
#endif
