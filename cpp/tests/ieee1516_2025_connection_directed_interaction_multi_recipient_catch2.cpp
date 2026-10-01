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
    "RTIambassadors deliver one directed interaction to multiple subscribed recipients through a configured process endpoint",
    "[integration][foundation][federation-management][interaction-management][object-management]"
    "[directed-interaction][directed-routing][process-directed-interaction-multi-recipient][callbacks][callback-immediate][transport][process-boundary][public-endpoint][multi-federate][2025]"
    "[process-event.receive-object-instance-discovery][process-event.receive-directed-interaction][federate.callback.receive-directed-interaction]") {
  auto runScenario = [](CallbackModel callbackModel) {
    class RecordingFederateAmbassador final : public NullFederateAmbassador {
     public:
      void discoverObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::ObjectClassHandle const& objectClass,
          std::wstring const& objectInstanceName,
          rti1516_2025::FederateHandle const& producingFederate) override {
        ++discoveryCount;
        discoveredObjectInstance = objectInstance;
        discoveredObjectClass = objectClass;
        discoveredObjectInstanceName = objectInstanceName;
        discoveredProducingFederate = producingFederate;
      }

      void receiveDirectedInteraction(
          rti1516_2025::InteractionClassHandle const& interactionClass,
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::ParameterHandleValueMap const& parameterValues,
          rti1516_2025::VariableLengthData const& userSuppliedTag,
          rti1516_2025::TransportationTypeHandle const& transportationType,
          rti1516_2025::FederateHandle const& producingFederate) override {
        ++receiveCount;
        receivedInteractionClass = interactionClass;
        receivedObjectInstance = objectInstance;
        receivedParameterCount = parameterValues.size();
        receivedTransportationType = transportationType;
        receivedProducingFederate = producingFederate;
        receivedTag.clear();
        if (userSuppliedTag.size() != 0U) {
          auto const* first =
              static_cast<std::uint8_t const*>(userSuppliedTag.data());
          receivedTag.assign(first, first + userSuppliedTag.size());
        }
      }

      std::size_t discoveryCount = 0U;
      rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
      rti1516_2025::ObjectClassHandle discoveredObjectClass;
      std::wstring discoveredObjectInstanceName;
      rti1516_2025::FederateHandle discoveredProducingFederate;
      std::size_t receiveCount = 0U;
      rti1516_2025::InteractionClassHandle receivedInteractionClass;
      rti1516_2025::ObjectInstanceHandle receivedObjectInstance;
      std::size_t receivedParameterCount = 0U;
      rti1516_2025::TransportationTypeHandle receivedTransportationType;
      rti1516_2025::FederateHandle receivedProducingFederate;
      std::vector<std::uint8_t> receivedTag;
    };

    constexpr wchar_t const* federationName =
        L"public-process-directed-multi-recipient-execution";
    constexpr wchar_t const* senderName =
        L"public-process-directed-multi-recipient-sender";
    constexpr wchar_t const* receiverOneName =
        L"public-process-directed-multi-recipient-receiver-one";
    constexpr wchar_t const* receiverTwoName =
        L"public-process-directed-multi-recipient-receiver-two";
    constexpr wchar_t const* objectClassName =
        L"HLAobjectRoot.UmbraDirectedFixtureObject";
    constexpr wchar_t const* attributeName = L"DirectedTargetMarker";
    constexpr wchar_t const* interactionClassName =
        L"HLAinteractionRoot.UmbraDirectedFixtureInteraction";

    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    std::atomic_uint64_t expectedObjectClass{0U};
    std::atomic_uint64_t expectedAttribute{0U};
    std::atomic_uint64_t expectedInteractionClass{0U};
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry,
            composedDirectedProcessDefinition(),
            ProcessFederationServiceOptions{callbackModel == HLA_IMMEDIATE});

        auto serveExpected = [&](ProcessTransportSession& session,
                                 auto const& handler,
                                 TransportServiceOperation operation,
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

        auto senderConnection = listener->accept(
            nullptr,
            {"public-process-directed-multi-recipient-server", 0x9721U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession sender(senderConnection);
        auto senderHandler = service.handlerFor(sender);
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::create_federation_execution,
            "The directed multi-recipient server lost Create.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::join_federation_execution,
            "The directed multi-recipient server lost sender Join.");

        auto const objectClass =
            registry.objectClassHandleFor(federationName, "HLAobjectRoot.UmbraDirectedFixtureObject");
        auto const attribute = registry.attributeHandleFor(
            federationName,
            "HLAobjectRoot.UmbraDirectedFixtureObject",
            "DirectedTargetMarker");
        auto const interactionClass = registry.interactionClassHandleFor(
            federationName,
            "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
        if (!objectClass || !attribute || !interactionClass) {
          throw std::runtime_error(
              "The directed multi-recipient server could not resolve its FOM handles.");
        }
        expectedObjectClass.store(*objectClass, std::memory_order_release);
        expectedAttribute.store(*attribute, std::memory_order_release);
        expectedInteractionClass.store(*interactionClass, std::memory_order_release);

        auto receiverOneConnection = listener->accept(
            nullptr,
            {"public-process-directed-multi-recipient-server", 0x9722U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession receiverOne(receiverOneConnection);
        auto receiverOneHandler = service.handlerFor(receiverOne);
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::join_federation_execution,
            "The directed multi-recipient server lost receiver-one Join.");

        auto receiverTwoConnection = listener->accept(
            nullptr,
            {"public-process-directed-multi-recipient-server", 0x9723U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession receiverTwo(receiverTwoConnection);
        auto receiverTwoHandler = service.handlerFor(receiverTwo);
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::join_federation_execution,
            "The directed multi-recipient server lost receiver-two Join.");

        auto serveLookups = [&](ProcessTransportSession& session,
                                auto const& handler,
                                char const* description) {
          serveExpected(
              session,
              handler,
              TransportServiceOperation::get_object_class_handle,
              description);
          serveExpected(
              session,
              handler,
              TransportServiceOperation::get_attribute_handle,
              description);
          serveExpected(
              session,
              handler,
              TransportServiceOperation::get_interaction_class_handle,
              description);
        };
        serveLookups(
            sender,
            senderHandler,
            "The directed multi-recipient server lost sender lookup.");
        serveLookups(
            receiverOne,
            receiverOneHandler,
            "The directed multi-recipient server lost receiver-one lookup.");
        serveLookups(
            receiverTwo,
            receiverTwoHandler,
            "The directed multi-recipient server lost receiver-two lookup.");

        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::publish_object_class_attributes,
            "The directed multi-recipient server lost target publication.");
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::subscribe_object_class_attributes,
            "The directed multi-recipient server lost receiver-one target subscription.");
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::subscribe_object_class_attributes,
            "The directed multi-recipient server lost receiver-two target subscription.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::publish_object_class_directed_interactions,
            "The directed multi-recipient server lost directed publication.");
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::subscribe_object_class_directed_interactions,
            "The directed multi-recipient server lost receiver-one directed subscription.");
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::subscribe_object_class_directed_interactions,
            "The directed multi-recipient server lost receiver-two directed subscription.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::register_object_instance,
            "The directed multi-recipient server lost target registration.");

        auto serveReceiverEvent = [&](ProcessTransportSession& session,
                                      auto const& handler,
                                      char const* description) {
          serveExpected(
              session,
              handler,
              callbackModel == HLA_IMMEDIATE
                  ? TransportServiceOperation::get_object_class_handle
                  : TransportServiceOperation::receive_interaction,
              description);
        };
        serveReceiverEvent(
            receiverOne,
            receiverOneHandler,
            "The directed multi-recipient server lost receiver-one discovery polling.");
        serveReceiverEvent(
            receiverTwo,
            receiverTwoHandler,
            "The directed multi-recipient server lost receiver-two discovery polling.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::send_directed_interaction,
            "The directed multi-recipient server lost directed Send.");
        serveReceiverEvent(
            receiverOne,
            receiverOneHandler,
            "The directed multi-recipient server lost receiver-one directed polling.");
        serveReceiverEvent(
            receiverTwo,
            receiverTwoHandler,
            "The directed multi-recipient server lost receiver-two directed polling.");

        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::resign_federation_execution,
            "The directed multi-recipient server lost sender Resign.");
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::resign_federation_execution,
            "The directed multi-recipient server lost receiver-one Resign.");
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::resign_federation_execution,
            "The directed multi-recipient server lost receiver-two Resign.");

        service.detach(sender);
        service.detach(receiverOne);
        service.detach(receiverTwo);
        senderConnection->close();
        receiverOneConnection->close();
        receiverTwoConnection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    RecordingFederateAmbassador senderFederate;
    RecordingFederateAmbassador receiverOneFederate;
    RecordingFederateAmbassador receiverTwoFederate;
    auto senderRti = makeRti();
    auto receiverOneRti = makeRti();
    auto receiverTwoRti = makeRti();
    auto senderConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"public-process-directed-multi-recipient-sender-client")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port));
    auto receiverOneConfiguration = RtiConfiguration::createConfiguration()
                                        .withConfigurationName(
                                            L"public-process-directed-multi-recipient-receiver-one-client")
                                        .withRtiAddress(
                                            L"tcp://127.0.0.1:" + std::to_wstring(port));
    auto receiverTwoConfiguration = RtiConfiguration::createConfiguration()
                                        .withConfigurationName(
                                            L"public-process-directed-multi-recipient-receiver-two-client")
                                        .withRtiAddress(
                                            L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool senderJoined = false;
    bool receiverOneJoined = false;
    bool receiverTwoJoined = false;
    try {
      REQUIRE(senderRti->connect(
                  senderFederate, callbackModel, senderConfiguration)
                  .addressUsed);
      senderRti->createFederationExecution(
          federationName, L"server-owned-directed-multi-recipient-fom.xml");
      auto const senderHandle = senderRti->joinFederationExecution(
          senderName,
          L"public-process-directed-multi-recipient-type",
          federationName);
      REQUIRE(senderHandle.isValid());
      senderJoined = true;

      REQUIRE(receiverOneRti->connect(
                  receiverOneFederate, callbackModel, receiverOneConfiguration)
                  .addressUsed);
      auto const receiverOneHandle = receiverOneRti->joinFederationExecution(
          receiverOneName,
          L"public-process-directed-multi-recipient-type",
          federationName);
      REQUIRE(receiverOneHandle.isValid());
      receiverOneJoined = true;

      REQUIRE(receiverTwoRti->connect(
                  receiverTwoFederate, callbackModel, receiverTwoConfiguration)
                  .addressUsed);
      auto const receiverTwoHandle = receiverTwoRti->joinFederationExecution(
          receiverTwoName,
          L"public-process-directed-multi-recipient-type",
          federationName);
      REQUIRE(receiverTwoHandle.isValid());
      receiverTwoJoined = true;

      auto const senderObjectClass = senderRti->getObjectClassHandle(objectClassName);
      auto const senderAttribute =
          senderRti->getAttributeHandle(senderObjectClass, attributeName);
      auto const senderInteraction =
          senderRti->getInteractionClassHandle(interactionClassName);
      auto const receiverOneObjectClass =
          receiverOneRti->getObjectClassHandle(objectClassName);
      auto const receiverOneAttribute = receiverOneRti->getAttributeHandle(
          receiverOneObjectClass, attributeName);
      auto const receiverOneInteraction =
          receiverOneRti->getInteractionClassHandle(interactionClassName);
      auto const receiverTwoObjectClass =
          receiverTwoRti->getObjectClassHandle(objectClassName);
      auto const receiverTwoAttribute = receiverTwoRti->getAttributeHandle(
          receiverTwoObjectClass, attributeName);
      auto const receiverTwoInteraction =
          receiverTwoRti->getInteractionClassHandle(interactionClassName);
      REQUIRE(senderObjectClass == receiverOneObjectClass);
      REQUIRE(senderObjectClass == receiverTwoObjectClass);
      REQUIRE(senderAttribute == receiverOneAttribute);
      REQUIRE(senderAttribute == receiverTwoAttribute);
      REQUIRE(senderInteraction == receiverOneInteraction);
      REQUIRE(senderInteraction == receiverTwoInteraction);
      REQUIRE(senderObjectClass ==
              rti1516_2025::umbra_binding_detail::makeObjectClassHandle(
                  expectedObjectClass.load(std::memory_order_acquire)));
      REQUIRE(senderAttribute ==
              rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                  expectedAttribute.load(std::memory_order_acquire)));
      REQUIRE(senderInteraction ==
              rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
                  expectedInteractionClass.load(std::memory_order_acquire)));

      REQUIRE_NOTHROW(senderRti->publishObjectClassAttributes(
          senderObjectClass,
          rti1516_2025::AttributeHandleSet{senderAttribute}));
      REQUIRE_NOTHROW(receiverOneRti->subscribeObjectClassAttributes(
          receiverOneObjectClass,
          rti1516_2025::AttributeHandleSet{receiverOneAttribute},
          true));
      REQUIRE_NOTHROW(receiverTwoRti->subscribeObjectClassAttributes(
          receiverTwoObjectClass,
          rti1516_2025::AttributeHandleSet{receiverTwoAttribute},
          true));
      REQUIRE_NOTHROW(senderRti->publishObjectClassDirectedInteractions(
          senderObjectClass,
          rti1516_2025::InteractionClassHandleSet{senderInteraction}));
      REQUIRE_NOTHROW(receiverOneRti->subscribeObjectClassDirectedInteractions(
          receiverOneObjectClass,
          rti1516_2025::InteractionClassHandleSet{receiverOneInteraction},
          true));
      REQUIRE_NOTHROW(receiverTwoRti->subscribeObjectClassDirectedInteractions(
          receiverTwoObjectClass,
          rti1516_2025::InteractionClassHandleSet{receiverTwoInteraction},
          true));

      auto const objectInstance =
          senderRti->registerObjectInstance(senderObjectClass);
      REQUIRE(objectInstance.isValid());
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(receiverOneRti->getObjectClassHandle(objectClassName));
        static_cast<void>(receiverTwoRti->getObjectClassHandle(objectClassName));
      } else {
        static_cast<void>(receiverOneRti->evokeCallback(0.0));
        static_cast<void>(receiverTwoRti->evokeCallback(0.0));
      }
      for (auto const* receiver : {&receiverOneFederate, &receiverTwoFederate}) {
        REQUIRE(receiver->discoveryCount == 1U);
        REQUIRE(receiver->discoveredObjectInstance == objectInstance);
        REQUIRE(receiver->discoveredObjectClass == senderObjectClass);
        REQUIRE_FALSE(receiver->discoveredObjectInstanceName.empty());
        REQUIRE(receiver->discoveredProducingFederate == senderHandle);
      }

      std::array<std::uint8_t, 3U> encodedTag{0x4DU, 0x52U, 0x32U};
      VariableLengthData userSuppliedTag(encodedTag.data(), encodedTag.size());
      REQUIRE_NOTHROW(senderRti->sendDirectedInteraction(
          senderInteraction,
          objectInstance,
          rti1516_2025::ParameterHandleValueMap{},
          userSuppliedTag));
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(receiverOneRti->getObjectClassHandle(objectClassName));
        static_cast<void>(receiverTwoRti->getObjectClassHandle(objectClassName));
      } else {
        static_cast<void>(receiverOneRti->evokeCallback(0.0));
        static_cast<void>(receiverTwoRti->evokeCallback(0.0));
      }
      auto const expectedTag = std::vector<std::uint8_t>{0x4DU, 0x52U, 0x32U};
      for (auto const* receiver : {&receiverOneFederate, &receiverTwoFederate}) {
        REQUIRE(receiver->receiveCount == 1U);
        REQUIRE(receiver->receivedInteractionClass == senderInteraction);
        REQUIRE(receiver->receivedObjectInstance == objectInstance);
        REQUIRE(receiver->receivedParameterCount == 0U);
        REQUIRE(receiver->receivedProducingFederate == senderHandle);
        REQUIRE(receiver->receivedTag == expectedTag);
        REQUIRE(receiver->receivedTransportationType.isValid());
      }
      REQUIRE(senderFederate.discoveryCount == 0U);
      REQUIRE(senderFederate.receiveCount == 0U);

      // The registered target still owns its attributes.  Divest them on
      // sender resignation so the directed-routing fixture can leave the
      // object known to both recipients without introducing a deletion
      // callback into this focused lane.
      senderRti->resignFederationExecution(
          rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
      senderJoined = false;
      receiverOneRti->resignFederationExecution(NO_ACTION);
      receiverOneJoined = false;
      receiverTwoRti->resignFederationExecution(NO_ACTION);
      receiverTwoJoined = false;
      senderRti->disconnect();
      receiverOneRti->disconnect();
      receiverTwoRti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      if (senderJoined) {
        try {
          senderRti->resignFederationExecution(
              rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
        } catch (...) {
        }
      }
      if (receiverOneJoined) {
        try {
          receiverOneRti->resignFederationExecution(NO_ACTION);
        } catch (...) {
        }
      }
      if (receiverTwoJoined) {
        try {
          receiverTwoRti->resignFederationExecution(NO_ACTION);
        } catch (...) {
        }
      }
      try {
        senderRti->disconnect();
      } catch (...) {
      }
      try {
        receiverOneRti->disconnect();
      } catch (...) {
      }
      try {
        receiverTwoRti->disconnect();
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
    REQUIRE_FALSE(receiverOneJoined);
    REQUIRE_FALSE(receiverTwoJoined);
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
