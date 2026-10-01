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
    "RTIambassador delivers object discovery through a configured process endpoint",
    "[integration][foundation][declaration-management][object-management][callbacks][callback-controls][transport][process-boundary][public-endpoint][rti.service.get-object-class-handle][rti.service.get-attribute-handle][rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes][rti.service.register-object-instance][federate.callback.discover-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
    class RecordingFederateAmbassador final : public NullFederateAmbassador {
     public:
      void discoverObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::ObjectClassHandle const& objectClass,
          std::wstring const& objectInstanceName,
          rti1516_2025::FederateHandle const& producingFederate) override {
        discoveredObjectInstance = objectInstance;
        discoveredObjectClass = objectClass;
        discoveredObjectInstanceName = objectInstanceName;
        discoveredProducingFederate = producingFederate;
        discovered.store(true, std::memory_order_release);
      }

      std::atomic_bool discovered{false};
      rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
      rti1516_2025::ObjectClassHandle discoveredObjectClass;
      std::wstring discoveredObjectInstanceName;
      rti1516_2025::FederateHandle discoveredProducingFederate;
    } receiverFederate;

    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    constexpr wchar_t const* federationName =
        L"public-process-object-discovery-execution";
    constexpr wchar_t const* objectClassNameWide = L"HLAobjectRoot.Customer";
    constexpr char const* objectClassName = "HLAobjectRoot.Customer";
    constexpr wchar_t const* attributeNameWide = L"HLAprivilegeToDeleteObject";
    constexpr char const* attributeName = "HLAprivilegeToDeleteObject";
    std::atomic_uint64_t expectedObjectClass{0U};
    std::atomic_uint64_t expectedAttribute{0U};
    std::atomic_uint64_t expectedObjectInstance{0U};
    std::atomic_uint64_t expectedSenderFederate{0U};
    std::atomic_uint64_t expectedReceiverFederate{0U};
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry,
            composedProcessDefinition(),
            ProcessFederationServiceOptions{callbackModel == HLA_IMMEDIATE});
        auto senderConnection = listener->accept(
            nullptr,
            {"public-process-object-discovery-server", 0xA901U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession sender(senderConnection);
        auto senderBaseHandler = service.handlerFor(sender);
        auto senderHandler = [&](TransportServiceMessage const& request) {
          auto response = senderBaseHandler(request);
          if (request.operation ==
                  TransportServiceOperation::join_federation_execution &&
              response.status == TransportServiceStatus::ok) {
            auto const result =
                umbra::detail::decodeProcessFederationJoinResult(response.payload);
            expectedSenderFederate.store(
                result.federateId, std::memory_order_release);
          }
          if (request.operation ==
                  TransportServiceOperation::register_object_instance &&
              response.status == TransportServiceStatus::ok) {
            auto const result =
                umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                    response.payload);
            expectedObjectInstance.store(
                result.objectInstanceHandle, std::memory_order_release);
          }
          return response;
        };
        auto serveExpected = [&](ProcessTransportSession& session,
                                 auto const& handler,
                                 TransportServiceOperation operation,
                                 char const* description) {
          bool expectedOperationServed = false;
          while (!expectedOperationServed) {
            if (!umbra::test::servePrimaryProcessRequest(
                    session, handler,
                    [&](TransportServiceMessage const& request) {
                      if (callbackModel == HLA_IMMEDIATE &&
                          request.operation ==
                              TransportServiceOperation::receive_interaction &&
                          operation !=
                              TransportServiceOperation::receive_interaction) {
                        return handler(request);
                      }
                      if (request.operation != operation) {
                        throw std::runtime_error(description);
                      }
                      expectedOperationServed = true;
                      return handler(request);
                    })) {
              throw std::runtime_error(description);
            }
          }
        };

        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::create_federation_execution,
            "The public process object-discovery server lost Create.");
        auto const objectClass = registry.objectClassHandleFor(
            federationName, objectClassName);
        auto const attribute = registry.attributeHandleFor(
            federationName, objectClassName, attributeName);
        if (!objectClass || !attribute) {
          throw std::runtime_error(
              "The public process object-discovery server could not resolve its FOM handles.");
        }
        expectedObjectClass.store(*objectClass, std::memory_order_release);
        expectedAttribute.store(*attribute, std::memory_order_release);
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::join_federation_execution,
            "The public process object-discovery server lost sender Join.");

        auto receiverConnection = listener->accept(
            nullptr,
            {"public-process-object-discovery-server", 0xA902U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession receiver(receiverConnection);
        auto receiverBaseHandler = service.handlerFor(receiver);
        auto receiverHandler = [&](TransportServiceMessage const& request) {
          auto response = receiverBaseHandler(request);
          if (request.operation ==
                  TransportServiceOperation::join_federation_execution &&
              response.status == TransportServiceStatus::ok) {
            auto const result =
                umbra::detail::decodeProcessFederationJoinResult(response.payload);
            expectedReceiverFederate.store(
                result.federateId, std::memory_order_release);
          }
          return response;
        };
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::join_federation_execution,
            "The public process object-discovery server lost receiver Join.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_object_class_handle,
            "The public process object-discovery server lost sender class lookup.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_attribute_handle,
            "The public process object-discovery server lost sender attribute lookup.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_object_class_handle,
            "The public process object-discovery server lost receiver class lookup.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_attribute_handle,
            "The public process object-discovery server lost receiver attribute lookup.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::publish_object_class_attributes,
            "The public process object-discovery server lost sender Publish.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::subscribe_object_class_attributes,
            "The public process object-discovery server lost receiver Subscribe.");
        auto const senderMember = registry.memberByName(
            federationName, L"public-process-object-discovery-sender");
        auto const receiverMember = registry.memberByName(
            federationName, L"public-process-object-discovery-receiver");
        auto const published = senderMember
                                   ? registry.publishedObjectClassAttributeHandles(
                                         federationName, senderMember->id, *objectClass)
                                   : std::nullopt;
        auto const subscription = receiverMember
                                      ? registry.objectClassAttributeDeclarationFor(
                                            federationName,
                                            receiverMember->id,
                                            *objectClass)
                                      : std::nullopt;
        if (!senderMember || !receiverMember || !published ||
            !published->contains(*attribute) || !subscription ||
            subscription->subscribedAttributes.find(*attribute) ==
                subscription->subscribedAttributes.end() ||
            !subscription->subscribedAttributes.at(*attribute)) {
          throw std::runtime_error(
              "The public process object-discovery declarations did not reach the registry.");
        }
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::register_object_instance,
            "The public process object-discovery server lost sender Register.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::receive_interaction,
            "The public process object-discovery server lost receiver discovery delivery.");
        while (callbackModel == HLA_IMMEDIATE &&
               !receiverFederate.discovered.load(std::memory_order_acquire)) {
          serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::receive_interaction,
              "The public process object-discovery server lost immediate callback polling.");
        }
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::resign_federation_execution,
            "The public process object-discovery server lost sender Resign.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::resign_federation_execution,
            "The public process object-discovery server lost receiver Resign.");
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
    TestFederateAmbassador senderFederate;
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-object-discovery-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool senderJoined = false;
    bool receiverJoined = false;
    try {
      REQUIRE(senderRti->connect(
                  senderFederate, callbackModel, configuration)
                  .addressUsed);
      senderRti->createFederationExecution(
          federationName, L"server-owned-fom.xml");
      auto const senderHandle = senderRti->joinFederationExecution(
          L"public-process-object-discovery-sender",
          L"public-process-object-discovery-type",
          federationName);
      senderJoined = true;
      REQUIRE(receiverRti->connect(
                  receiverFederate, callbackModel, configuration)
                  .addressUsed);
      auto const receiverHandle = receiverRti->joinFederationExecution(
          L"public-process-object-discovery-receiver",
          L"public-process-object-discovery-type",
          federationName);
      receiverJoined = true;

      auto const senderObjectClass =
          senderRti->getObjectClassHandle(objectClassNameWide);
      auto const senderAttribute =
          senderRti->getAttributeHandle(senderObjectClass, attributeNameWide);
      REQUIRE(senderObjectClass.toString() ==
              L"ObjectClassHandle(" +
                  std::to_wstring(
                      expectedObjectClass.load(std::memory_order_acquire)) +
                  L")");
      REQUIRE(senderAttribute ==
              rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                  expectedAttribute.load(std::memory_order_acquire)));
      auto const receiverObjectClass =
          receiverRti->getObjectClassHandle(objectClassNameWide);
      auto const receiverAttribute =
          receiverRti->getAttributeHandle(receiverObjectClass, attributeNameWide);
      REQUIRE(receiverObjectClass == senderObjectClass);
      REQUIRE(receiverAttribute == senderAttribute);
      rti1516_2025::AttributeHandleSet senderAttributes;
      senderAttributes.insert(senderAttribute);
      REQUIRE_NOTHROW(senderRti->publishObjectClassAttributes(
          senderObjectClass, senderAttributes));
      rti1516_2025::AttributeHandleSet receiverAttributes;
      receiverAttributes.insert(receiverAttribute);
      REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributes(
          receiverObjectClass, receiverAttributes));
      auto const objectInstance =
          senderRti->registerObjectInstance(senderObjectClass);
      REQUIRE(objectInstance.isValid());
      REQUIRE(objectInstance.toString() ==
              L"ObjectInstanceHandle(" +
                  std::to_wstring(
                      expectedObjectInstance.load(std::memory_order_acquire)) +
                  L")");
      if (callbackModel == HLA_IMMEDIATE) {
        auto const deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!receiverFederate.discovered.load(std::memory_order_acquire) &&
               std::chrono::steady_clock::now() < deadline) {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      } else {
        for (std::size_t evokeCount = 0U;
             evokeCount < 4U &&
             !receiverFederate.discovered.load(std::memory_order_acquire);
             ++evokeCount) {
          static_cast<void>(receiverRti->evokeCallback(0.0));
        }
      }
      REQUIRE(receiverFederate.discovered.load(std::memory_order_acquire));
      REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);
      REQUIRE(receiverFederate.discoveredObjectClass == receiverObjectClass);
      REQUIRE_FALSE(receiverFederate.discoveredObjectInstanceName.empty());
      REQUIRE(receiverFederate.discoveredProducingFederate == senderHandle);
      REQUIRE(senderHandle.toString() ==
              L"FederateHandle(" +
                  std::to_wstring(expectedSenderFederate.load(
                      std::memory_order_acquire)) +
                  L")");
      REQUIRE(receiverHandle.toString() ==
              L"FederateHandle(" +
                  std::to_wstring(expectedReceiverFederate.load(
                      std::memory_order_acquire)) +
                  L")");
      senderRti->resignFederationExecution(DELETE_OBJECTS);
      senderJoined = false;
      receiverRti->resignFederationExecution(NO_ACTION);
      receiverJoined = false;
      senderRti->disconnect();
      receiverRti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      if (senderJoined) {
        try {
          senderRti->resignFederationExecution(DELETE_OBJECTS);
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

#endif
#endif
