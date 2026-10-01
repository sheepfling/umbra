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
class DirectedInteractionTransportationFederateAmbassador final
    : public NullFederateAmbassador {
 public:
  void confirmInteractionTransportationTypeChange(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::TransportationTypeHandle const& transportationType) override {
    ++changeCount;
    changedInteractionClass = interactionClass;
    changedTransportationType = transportationType;
  }

  std::size_t changeCount = 0U;
  rti1516_2025::InteractionClassHandle changedInteractionClass;
  rti1516_2025::TransportationTypeHandle changedTransportationType;
};

TEST_CASE(
    "RTIambassadors preserve a directed interaction transportation override through a configured process endpoint",
    "[integration][foundation][interaction-management][object-management][transportation]"
    "[directed-interaction][directed-routing][process-boundary][public-endpoint]"
    "[process-directed-interaction-transportation][callbacks][callback-immediate][2025]"
    "[rti.service.publish-object-class-directed-interactions][rti.service.request-interaction-transportation-type-change]"
    "[rti.service.send-directed-interaction][process-event.receive-directed-interaction]"
    "[federate.callback.confirm-interaction-transportation-type-change]") {
  auto runScenario = [](CallbackModel callbackModel) {
    constexpr wchar_t const* federationName =
        L"public-process-directed-transportation-execution";
    constexpr wchar_t const* senderName =
        L"public-process-directed-transportation-sender";
    constexpr wchar_t const* receiverName =
        L"public-process-directed-transportation-receiver";
    constexpr char const* objectClassName =
        "HLAobjectRoot.UmbraDirectedFixtureObject";
    constexpr char const* interactionClassName =
        "HLAinteractionRoot.UmbraDirectedFixtureInteraction";
    constexpr char const* attributeName = "DirectedTargetMarker";
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

        auto senderConnection = listener->accept(
            nullptr,
            {"public-process-directed-transportation-server", 0x9731U},
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
            sender,
            senderHandler,
            TransportServiceOperation::create_federation_execution,
            "The directed transportation server lost Create.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::join_federation_execution,
            "The directed transportation server lost sender Join.");

        auto const objectClass =
            registry.objectClassHandleFor(federationName, objectClassName);
        auto const interactionClass =
            registry.interactionClassHandleFor(federationName, interactionClassName);
        auto const attribute =
            registry.attributeHandleFor(federationName, objectClassName, attributeName);
        auto const transportation = registry.transportationTypeHandleFor(
            federationName, transportationTypeName);
        if (!objectClass || !interactionClass || !attribute || !transportation) {
          throw std::runtime_error(
              "The directed transportation server could not resolve its FOM handles.");
        }
        expectedObjectClass.store(*objectClass, std::memory_order_release);
        expectedInteractionClass.store(*interactionClass, std::memory_order_release);
        expectedTransportation.store(*transportation, std::memory_order_release);

        auto receiverConnection = listener->accept(
            nullptr,
            {"public-process-directed-transportation-server", 0x9732U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession receiver(receiverConnection);
        auto receiverHandler = service.handlerFor(receiver);
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::join_federation_execution,
            "The directed transportation server lost receiver Join.");

        auto const senderMember = registry.memberByName(federationName, senderName);
        auto const receiverMember = registry.memberByName(federationName, receiverName);
        if (!senderMember || !receiverMember) {
          throw std::runtime_error(
              "The directed transportation server could not resolve joined federates.");
        }
        if (registry.setObjectClassAttributePublication(
                federationName,
                senderMember->id,
                *objectClass,
                std::set<std::uint64_t>{*attribute},
                true) != ObjectClassAttributeDeclarationStatus::applied ||
            registry.setObjectClassAttributeSubscription(
                federationName,
                receiverMember->id,
                *objectClass,
                std::set<std::uint64_t>{*attribute},
                true) != ObjectClassAttributeDeclarationStatus::applied ||
            registry.subscribeObjectClassDirectedInteractions(
                federationName,
                receiverMember->id,
                *objectClass,
                std::set<std::uint64_t>{*interactionClass},
                true) != umbra::detail::DirectedInteractionDeclarationStatus::applied) {
          throw std::runtime_error(
              "The directed transportation target declarations were rejected.");
        }

        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_object_class_handle,
            "The directed transportation server lost object lookup.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_interaction_class_handle,
            "The directed transportation server lost interaction lookup.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_transportation_type_handle,
            "The directed transportation server lost transportation lookup.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::publish_object_class_directed_interactions,
            "The directed transportation server lost directed publication.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::request_interaction_transportation_type_change,
            "The directed transportation server lost directed transportation change.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::receive_interaction,
            "The directed transportation server lost transportation confirmation.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::register_object_instance,
            "The directed transportation server lost target registration.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::receive_interaction,
            "The directed transportation server lost target discovery.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::send_directed_interaction,
            "The directed transportation server lost directed Send.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::receive_interaction,
            "The directed transportation server lost directed Receive.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::resign_federation_execution,
            "The directed transportation server lost sender Resign.");
        service.detach(sender);
        service.detach(receiver);
        senderConnection->close();
        receiverConnection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    DirectedInteractionTransportationFederateAmbassador senderFederate;
    auto senderRti = makeRti();
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-directed-transportation-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    std::shared_ptr<ProcessTransportConnection> receiverConnection;
    std::optional<rti1516_2025::FederateHandle> senderJoined;
    std::optional<rti1516_2025::FederateHandle> receiverJoined;
    bool senderIsJoined = false;
    try {
      REQUIRE(senderRti->connect(senderFederate, callbackModel, configuration).addressUsed);
      senderRti->createFederationExecution(
          federationName, L"server-owned-directed-transportation-fom.xml");
      senderJoined = senderRti->joinFederationExecution(
          senderName,
          L"public-process-directed-transportation-type",
          federationName);
      senderIsJoined = true;

      receiverConnection = ProcessTransportConnection::connectClient(
          nullptr,
          {"127.0.0.1", port},
          {"public-process-directed-transportation-server", 0x9733U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      TransportServiceMessage receiverJoinResponse;
      REQUIRE(receiver.request(
                  TransportServiceMessage{
                      TransportServiceMessageKind::request,
                      TransportServiceOperation::join_federation_execution,
                      TransportServiceStatus::ok,
                      1U,
                      umbra::detail::encodeProcessFederationJoinRequest(
                          umbra::detail::ProcessFederationJoinRequest{
                              federationName,
                              L"public-process-directed-transportation-type",
                              receiverName})},
                  receiverJoinResponse));
      REQUIRE(receiverJoinResponse.status == TransportServiceStatus::ok);
      auto const receiverJoin = umbra::detail::decodeProcessFederationJoinResult(
          receiverJoinResponse.payload);
      receiverJoined = rti1516_2025::umbra_binding_detail::makeFederateHandle(
          receiverJoin.federateId);

      auto const objectClass = senderRti->getObjectClassHandle(
          L"HLAobjectRoot.UmbraDirectedFixtureObject");
      auto const interactionClass = senderRti->getInteractionClassHandle(
          L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");
      auto const transportation = senderRti->getTransportationTypeHandle(
          L"HLAbestEffort");
      REQUIRE(objectClass.toString() ==
              L"ObjectClassHandle(" +
                  std::to_wstring(expectedObjectClass.load(std::memory_order_acquire)) +
                  L")");
      REQUIRE(interactionClass.toString() ==
              L"InteractionClassHandle(" +
                  std::to_wstring(expectedInteractionClass.load(std::memory_order_acquire)) +
                  L")");
      REQUIRE(transportation.toString() ==
              L"TransportationTypeHandle(" +
                  std::to_wstring(expectedTransportation.load(std::memory_order_acquire)) +
                  L")");
      REQUIRE_NOTHROW(senderRti->publishObjectClassDirectedInteractions(
          objectClass, rti1516_2025::InteractionClassHandleSet{interactionClass}));
      REQUIRE_NOTHROW(senderRti->requestInteractionTransportationTypeChange(
          interactionClass, transportation));
      static_cast<void>(senderRti->evokeCallback(0.0));
      REQUIRE(senderFederate.changeCount == 1U);
      REQUIRE(senderFederate.changedInteractionClass == interactionClass);
      REQUIRE(senderFederate.changedTransportationType == transportation);

      auto const objectInstance = senderRti->registerObjectInstance(objectClass);
      REQUIRE(objectInstance.isValid());
      auto const objectInstanceValue =
          *rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(objectInstance);

      TransportServiceMessage discoveryResponse;
      REQUIRE(receiver.request(
          TransportServiceMessage{
              TransportServiceMessageKind::request,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              2U,
              umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                  umbra::detail::ProcessFederationReceiveInteractionRequest{
                      federationName, receiverJoin.federateId})},
          discoveryResponse));
      REQUIRE(discoveryResponse.status == TransportServiceStatus::ok);
      auto const discoveryResult =
          umbra::detail::decodeProcessFederationReceiveInteractionResult(
              discoveryResponse.payload);
      REQUIRE(discoveryResult.discoveryEvent.has_value());
      REQUIRE(discoveryResult.discoveryEvent->objectInstanceHandle == objectInstanceValue);

      std::array<std::uint8_t, 3U> const tagBytes{0x54U, 0x52U, 0x4EU};
      REQUIRE_NOTHROW(senderRti->sendDirectedInteraction(
          interactionClass,
          objectInstance,
          ParameterHandleValueMap{},
          VariableLengthData(tagBytes.data(), tagBytes.size())));

      TransportServiceMessage receiveResponse;
      REQUIRE(receiver.request(
          TransportServiceMessage{
              TransportServiceMessageKind::request,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              3U,
              umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                  umbra::detail::ProcessFederationReceiveInteractionRequest{
                      federationName, receiverJoin.federateId})},
          receiveResponse));
      REQUIRE(receiveResponse.status == TransportServiceStatus::ok);
      auto const eventResult =
          umbra::detail::decodeProcessFederationReceiveInteractionResult(
              receiveResponse.payload);
      REQUIRE(eventResult.event.has_value());
      REQUIRE(eventResult.event->objectInstanceHandle.has_value());
      REQUIRE(*eventResult.event->objectInstanceHandle == objectInstanceValue);
      REQUIRE(eventResult.event->transportationName == transportationTypeName);
      REQUIRE(eventResult.event->interactionClassHandle ==
              rti1516_2025::umbra_binding_detail::interactionClassHandleValue(
                  interactionClass));
      auto const envelope = umbra::detail::decodeProcessFederationInteractionEnvelope(
          eventResult.event->payload);
      REQUIRE(envelope.has_value());
      REQUIRE(envelope->parameterValues.empty());
      REQUIRE(envelope->userSuppliedTag ==
              std::vector<std::uint8_t>{0x54U, 0x52U, 0x4EU});

      senderRti->resignFederationExecution(DELETE_OBJECTS);
      senderIsJoined = false;
      senderRti->disconnect();
      receiverConnection->close();
    } catch (...) {
      clientError = std::current_exception();
      if (senderIsJoined) {
        try {
          senderRti->resignFederationExecution(NO_ACTION);
        } catch (...) {
        }
      }
      try {
        senderRti->disconnect();
      } catch (...) {
      }
      if (receiverConnection) {
        receiverConnection->close();
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
        FAIL_CHECK(std::string("directed transportation server: ") + error.what());
      } catch (...) {
        FAIL_CHECK("directed transportation server failed with an unknown exception");
      }
    }
    if (clientError) {
      std::rethrow_exception(clientError);
    }
    REQUIRE_FALSE(serverError);
    REQUIRE_FALSE(senderIsJoined);
    REQUIRE(receiverJoined.has_value());
    REQUIRE(receiverJoined->isValid());
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
