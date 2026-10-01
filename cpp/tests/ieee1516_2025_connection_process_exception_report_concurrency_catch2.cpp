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
#include <RTI/encoding/HLAvariableArray.h>

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
    "RTIambassador preserves process exception-report recheck response ownership during concurrent service requests",
    "[integration][development-profile][mom][transport][process-boundary][public-endpoint][2025]"
    "[process-exception-report-concurrency]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-object-class-handle]"
    "[rti.service.get-object-class-name][rti.service.get-interaction-class-handle]"
    "[rti.service.get-parameter-handle][rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.set-exception-reporting-switch]"
    "[rti.service.get-exception-reporting-switch][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]"
    "[federate.callback.receive-interaction]") {
  // Gates order operations, not wall-clock sleeps. Deadlines only bound failures.
  struct Gate {
    std::mutex mutex;
    std::condition_variable changed;
    bool opened = false;
    void open() {
      std::lock_guard lock(mutex);
      opened = true;
      changed.notify_all();
    }
    void wait() {
      std::unique_lock lock(mutex);
      if (!changed.wait_for(lock, std::chrono::seconds(5), [&] { return opened; })) {
        throw std::runtime_error("Timed out at a process-report concurrency gate.");
      }
    }
  };
  struct Report {
    rti1516_2025::InteractionClassHandle interaction;
    ParameterHandleValueMap parameters;
    rti1516_2025::TransportationTypeHandle transportation;
    rti1516_2025::FederateHandle producer;
    std::size_t tagSize;
    bool sentRegions;
  };
  class Observer final : public NullFederateAmbassador {
   public:
    void receiveInteraction(
        rti1516_2025::InteractionClassHandle const& interaction,
        ParameterHandleValueMap const& parameters,
        VariableLengthData const& tag,
        rti1516_2025::TransportationTypeHandle const& transportation,
        rti1516_2025::FederateHandle const& producer,
        rti1516_2025::RegionHandleSet const* regions) override {
      reports.push_back({interaction, parameters, transportation, producer,
                         tag.size(), regions != nullptr});
      if (reports.size() == 1U && firstCallback) {
        try { firstCallback(); } catch (...) { callbackError = std::current_exception(); }
      }
    }
    std::vector<Report> reports;
    std::function<void()> firstCallback;
    std::exception_ptr callbackError;
  };

  auto runScenario = [&](bool immediate, bool pushEvents, bool queuedResign = false) {
    Gate recheckEntered;
    Gate releaseRecheck;
    Gate lookupStarted;
    Gate callbackEntered;
    Gate secondReportProcessed;
    std::atomic<bool> holdRecheck{false};
    std::atomic<std::size_t> reportRequests{0U};
    std::atomic<std::size_t> recheckRequests{0U};
    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);
    std::mutex connectionMutex;
    std::shared_ptr<ProcessTransportConnection> serverConnection;
    std::exception_ptr serverError;
    std::vector<std::uint64_t> requestIds;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry, composedProcessDefinition(), ProcessFederationServiceOptions{pushEvents});
        auto connection = listener->accept(
            nullptr, {"exception-report-concurrency-server", 0x9E25U},
            [](std::wstring) {}, [](std::wstring) { return false; });
        {
          std::lock_guard lock(connectionMutex);
          serverConnection = connection;
        }
        ProcessTransportSession session(connection);
        auto handler = service.handlerFor(session);
        while (ProcessTransportServiceDispatcher::serveOne(
            session, [&](TransportServiceMessage const& request) {
              requestIds.push_back(request.requestId);
              if (request.operation == TransportServiceOperation::recheck_exception_report) {
                ++recheckRequests;
                if (holdRecheck.exchange(false)) {
                  recheckEntered.open();
                  releaseRecheck.wait();
                }
              }
              auto response = handler(request);
              if (request.operation == TransportServiceOperation::report_service_exception &&
                  ++reportRequests == 2U) {
                secondReportProcessed.open();
              }
              return response;
            })) {}
        service.detach(session);
        connection->close();
      } catch (...) {
        serverError = std::current_exception();
        std::lock_guard lock(connectionMutex);
        if (serverConnection) { serverConnection->close(); }
      }
    });

    Observer observer;
    auto rti = makeRti();
    auto configuration = RtiConfiguration::createConfiguration()
        .withConfigurationName(L"process-report-concurrency-client")
        .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::future<void> first;
    std::future<void> second;
    std::exception_ptr clientError;
    bool joined = false;
    auto stopServer = [&] {
      releaseRecheck.open();
      secondReportProcessed.open();
      std::lock_guard lock(connectionMutex);
      if (serverConnection) { serverConnection->close(); }
    };
    auto finish = [&](std::future<void>& operation) {
      if (!operation.valid()) {
        return;
      }
      if (operation.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
        stopServer();
        throw std::runtime_error("Concurrent public RTI operation did not finish.");
      }
      operation.get();
    };
    try {
      REQUIRE(rti->connect(observer, immediate ? HLA_IMMEDIATE : HLA_EVOKED,
                           configuration).addressUsed);
      rti->createFederationExecution(L"report-concurrency", L"server-owned-fom.xml");
      auto const federate = rti->joinFederationExecution(
          L"concurrent-observer", L"test", L"report-concurrency");
      joined = true;
      auto const rootClass = rti->getObjectClassHandle(L"HLAobjectRoot");
      auto const reportClass = rti->getInteractionClassHandle(
          L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportException");
      auto const serviceParameter = rti->getParameterHandle(reportClass, L"HLAservice");
      auto const exceptionParameter = rti->getParameterHandle(reportClass, L"HLAexception");
      auto const federateParameter = rti->getParameterHandle(reportClass, L"HLAfederate");
      auto const reliable = rti->getTransportationTypeHandle(L"HLAreliable");
      rti->subscribeInteractionClass(reportClass, true);
      rti->setExceptionReportingSwitch(true);
      std::atomic<std::size_t> typedFailures{0U};
      std::atomic<std::size_t> successfulLookups{0U};
      auto failLookup = [&] {
        try {
          static_cast<void>(rti->getObjectClassHandle(L"HLAobjectRoot.MissingConcurrentClass"));
        } catch (rti1516_2025::NameNotFound const&) {
          ++typedFailures;
          return;
        }
        throw std::runtime_error("Concurrent lookup lost its original NameNotFound exception.");
      };
      auto successfulLookup = [&] {
        if (rti->getObjectClassHandle(L"HLAobjectRoot") != rootClass ||
            rti->getObjectClassName(rootClass) != L"HLAobjectRoot" ||
            !rti->getExceptionReportingSwitch()) {
          throw std::runtime_error("A concurrent request consumed another operation's response.");
        }
        ++successfulLookups;
      };
      // Join before the lambdas and values captured by workers leave scope,
      // including when a gate or an assertion fails during setup.
      struct WorkerGuard {
        std::future<void>& first;
        std::future<void>& second;
        std::function<void()> cancel;
        ~WorkerGuard() {
          if (first.valid() || second.valid()) {
            cancel();
            if (first.valid()) { first.wait(); }
            if (second.valid()) { second.wait(); }
          }
        }
      } workerGuard{first, second, stopServer};
      if (queuedResign) {
        failLookup();
        REQUIRE(observer.reports.empty());
        REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
        joined = false;
      } else if (!immediate) {
        failLookup();
        REQUIRE(observer.reports.empty());
        holdRecheck = true;
        first = std::async(std::launch::async, [&] { static_cast<void>(rti->evokeCallback(0.0)); });
        recheckEntered.wait();
        second = std::async(std::launch::async, [&] {
          lookupStarted.open();
          successfulLookup();
        });
        lookupStarted.wait();
        releaseRecheck.open();
      } else {
        // The first immediate callback owns the dispatcher while the second
        // public call produces a pushed report. A connection lock held across
        // callback dispatch deadlocks when this callback makes its nested call.
        observer.firstCallback = [&] {
          callbackEntered.open();
          secondReportProcessed.wait();
          successfulLookup();
        };
        first = std::async(std::launch::async, failLookup);
        callbackEntered.wait();
        second = std::async(std::launch::async, failLookup);
      }
      finish(first);
      finish(second);
      while (rti->evokeCallback(0.0)) {}
      REQUIRE_FALSE(observer.callbackError);
      auto const expectedReports = immediate ? 2U : queuedResign ? 0U : 1U;
      REQUIRE(successfulLookups == (queuedResign ? 0U : 1U));
      REQUIRE(typedFailures == (immediate ? 2U : 1U));
      REQUIRE(reportRequests == (queuedResign ? 1U : expectedReports));
      REQUIRE(recheckRequests == (queuedResign ? 0U : expectedReports));
      REQUIRE(observer.reports.size() == expectedReports);
      for (auto const& report : observer.reports) {
        REQUIRE(report.interaction == reportClass);
        REQUIRE(report.parameters.size() == 3U);
        REQUIRE(report.transportation == reliable);
        REQUIRE_FALSE(report.producer.isValid());
        REQUIRE(report.tagSize == 0U);
        REQUIRE_FALSE(report.sentRegions);
        rti1516_2025::HLAunicodeString service;
        REQUIRE_NOTHROW(service.decode(report.parameters.at(serviceParameter)));
        REQUIRE(service.get() == L"Get Object Class Handle");
        rti1516_2025::HLAunicodeString exception;
        REQUIRE_NOTHROW(exception.decode(report.parameters.at(exceptionParameter)));
        REQUIRE(exception.get().find(L"NameNotFound") != std::wstring::npos);
        auto const encodedFederate = federate.encode();
        auto const& reportedFederate = report.parameters.at(federateParameter);
        REQUIRE(reportedFederate.size() == encodedFederate.size());
        REQUIRE(std::memcmp(reportedFederate.data(), encodedFederate.data(),
                            encodedFederate.size()) == 0);
      }
      if (!queuedResign) {
        rti->resignFederationExecution(NO_ACTION);
        joined = false;
      }
      rti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      stopServer();
      if (first.valid()) { first.wait(); }
      if (second.valid()) { second.wait(); }
      if (joined) { try { rti->resignFederationExecution(NO_ACTION); } catch (...) {} }
      try { rti->disconnect(); } catch (...) {}
    }
    listener.reset();
    server.join();
    if (clientError) { std::rethrow_exception(clientError); }
    REQUIRE_FALSE(serverError);
    REQUIRE_FALSE(joined);
    REQUIRE_FALSE(requestIds.empty());
    REQUIRE(std::adjacent_find(requestIds.begin(), requestIds.end(),
                              std::greater_equal<std::uint64_t>{}) == requestIds.end());
  };
  SECTION("Evoked report recheck overlaps a public request with pull delivery") {
    runScenario(false, false);
  }
  SECTION("Evoked report recheck overlaps a public request with push delivery") {
    runScenario(false, true);
  }
  SECTION("Immediate callback reenters while another request produces a report") {
    runScenario(true, true);
  }
  SECTION("Resign invalidates an undelivered evoked exception report") {
    runScenario(false, true, true);
  }
}

#endif
#endif
