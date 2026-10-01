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
    "RTIambassadors deliver multiple queued timestamped process messages in order through Flush Queue",
    "[integration][foundation][interaction-management][time-management][flush-queue-request][timestamped-interaction][tso][retraction][callback-immediate][process-boundary][process-flush-queue-multiple-records][process-flush-queue-retraction][public-endpoint][multi-federate][rti.service.send-interaction][rti.service.retract][rti.service.enable-time-regulation][rti.service.enable-time-constrained][rti.service.time-advance-request][rti.service.flush-queue-request][rti.service.query-logical-time][federate.callback.receive-interaction][federate.callback.flush-queue-grant]") {
  auto runScenario = [](CallbackModel callbackModel) {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void receiveInteraction(
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        rti1516_2025::VariableLengthData const&,
        rti1516_2025::TransportationTypeHandle const&,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const*,
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::OrderType sentOrder,
        rti1516_2025::OrderType receivedOrder,
        rti1516_2025::MessageRetractionHandle const*) override {
      ++receivedInteractionCount;
      callbackOrder.push_back('I');
      receivedInteractionClass = interactionClass;
      receivedProducingFederate = producingFederate;
      receivedParameterCount = parameterValues.size();
      receivedSentOrder = sentOrder;
      receivedReceivedOrder = receivedOrder;
      if (auto const* integerTime =
              dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
        receivedInteractionTimes.push_back(integerTime->getTime());
      }
    }

    void timeRegulationEnabled(rti1516_2025::LogicalTime const&) override {
      ++timeRegulationEnabledCount;
    }

    void timeConstrainedEnabled(rti1516_2025::LogicalTime const&) override {
      ++timeConstrainedEnabledCount;
    }

    void timeAdvanceGrant(rti1516_2025::LogicalTime const&) override {
      ++timeAdvanceGrantCount;
    }

    void flushQueueGrant(
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::LogicalTime const& optimisticTime) override {
      ++flushQueueGrantCount;
      callbackOrder.push_back('G');
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

    std::size_t receivedInteractionCount = 0U;
    std::vector<std::int64_t> receivedInteractionTimes;
    rti1516_2025::InteractionClassHandle receivedInteractionClass;
    rti1516_2025::FederateHandle receivedProducingFederate;
    std::size_t receivedParameterCount = 0U;
    rti1516_2025::OrderType receivedSentOrder = rti1516_2025::RECEIVE;
    rti1516_2025::OrderType receivedReceivedOrder = rti1516_2025::RECEIVE;
    std::size_t timeRegulationEnabledCount = 0U;
    std::size_t timeConstrainedEnabledCount = 0U;
    std::size_t timeAdvanceGrantCount = 0U;
    std::size_t flushQueueGrantCount = 0U;
    std::wstring flushQueueGrantImplementation;
    std::wstring flushQueueGrantOptimisticImplementation;
    std::int64_t flushQueueGrantValue = -1;
    std::int64_t flushQueueGrantOptimisticValue = -1;
    std::vector<char> callbackOrder;
  };

  constexpr wchar_t const* federationName =
      L"process-flush-queue-multiple-execution";
  constexpr wchar_t const* senderName = L"process-flush-queue-multiple-sender";
  constexpr wchar_t const* receiverName = L"process-flush-queue-multiple-receiver";
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
          {"process-flush-queue-multiple-server", 0x9216U},
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
          "The multiple-record FQR server lost Create.");
      auto const interactionClass = registry.interactionClassHandleFor(
          federationName, interactionName);
      auto const parameter = registry.parameterHandleFor(
          federationName, interactionName, parameterName);
      if (!interactionClass || !parameter) {
        throw std::runtime_error(
            "The multiple-record FQR server could not resolve its FOM handles.");
      }
      expectedInteractionClass.store(*interactionClass, std::memory_order_release);
      expectedParameter.store(*parameter, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The multiple-record FQR server lost sender Join.");

      auto receiverConnection = listener->accept(
          nullptr,
          {"process-flush-queue-multiple-server", 0x9217U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The multiple-record FQR server lost receiver Join.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The multiple-record FQR server lost sender interaction lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The multiple-record FQR server lost receiver interaction lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_parameter_handle,
          "The multiple-record FQR server lost parameter lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_interaction_class,
          "The multiple-record FQR server lost Publish.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_interaction_class,
          "The multiple-record FQR server lost Subscribe.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::enable_time_regulation,
          "The multiple-record FQR server lost Enable Time Regulation.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::enable_time_constrained,
          "The multiple-record FQR server lost Enable Time Constrained.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::send_interaction,
          "The multiple-record FQR server lost first Send Interaction.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::send_interaction,
          "The multiple-record FQR server lost second Send Interaction.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::send_interaction,
          "The multiple-record FQR server lost frontier Send Interaction.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::send_interaction,
          "The multiple-record FQR server lost future Send Interaction.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::send_interaction,
          "The multiple-record FQR server lost retractable future Send Interaction.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::retract,
          "The multiple-record FQR server lost queued TSO Retract.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The multiple-record FQR server lost sender TAR.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::flush_queue_request,
          "The multiple-record FQR server lost receiver FQR.");
      for (auto const* description : {
               "The multiple-record FQR server lost first TSO acknowledgement.",
               "The multiple-record FQR server lost second TSO acknowledgement.",
               "The multiple-record FQR server lost frontier TSO acknowledgement."}) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::acknowledge_tso_delivery,
            description);
      }
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::query_logical_time,
          "The multiple-record FQR server lost Query Logical Time.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::flush_queue_request,
          "The multiple-record FQR server lost second receiver FQR.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::acknowledge_tso_delivery,
          "The multiple-record FQR server lost retained TSO acknowledgement.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::query_logical_time,
          "The multiple-record FQR server lost second Query Logical Time.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The multiple-record FQR server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The multiple-record FQR server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  RecordingFederateAmbassador senderFederate;
  RecordingFederateAmbassador receiverFederate;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto senderConfiguration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"process-flush-queue-multiple-sender-client")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto receiverConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"process-flush-queue-multiple-receiver-client")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  rti1516_2025::FederateHandle senderHandle;
  try {
    REQUIRE(senderRti->connect(
                senderFederate, callbackModel, senderConfiguration)
                .addressUsed);
    REQUIRE_NOTHROW(senderRti->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    senderHandle = senderRti->joinFederationExecution(
        senderName, L"process-flush-queue-multiple-type", federationName);
    REQUIRE(senderHandle.isValid());
    senderJoined = true;
    REQUIRE(receiverRti->connect(
                receiverFederate, callbackModel, receiverConfiguration)
                .addressUsed);
    auto const receiverHandle = receiverRti->joinFederationExecution(
        receiverName, L"process-flush-queue-multiple-type", federationName);
    REQUIRE(receiverHandle.isValid());
    receiverJoined = true;

    auto const senderInteraction =
        senderRti->getInteractionClassHandle(interactionNameWide);
    auto const receiverInteraction =
        receiverRti->getInteractionClassHandle(interactionNameWide);
    auto const expectedClass =
        rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
            expectedInteractionClass.load(std::memory_order_acquire));
    REQUIRE(senderInteraction == expectedClass);
    REQUIRE(receiverInteraction == expectedClass);
    auto const parameter = senderRti->getParameterHandle(
        senderInteraction, parameterNameWide);
    REQUIRE(parameter ==
            rti1516_2025::umbra_binding_detail::makeParameterHandle(
                expectedParameter.load(std::memory_order_acquire)));
    REQUIRE_NOTHROW(senderRti->publishInteractionClass(senderInteraction));
    REQUIRE_NOTHROW(
        receiverRti->subscribeInteractionClass(receiverInteraction, true));
    REQUIRE_NOTHROW(senderRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(0)));
    if (callbackModel == HLA_EVOKED) {
      static_cast<void>(senderRti->evokeCallback(0.0));
    }
    REQUIRE(senderFederate.timeRegulationEnabledCount == 1U);
    REQUIRE_NOTHROW(receiverRti->enableTimeConstrained());
    if (callbackModel == HLA_EVOKED) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.timeConstrainedEnabledCount == 1U);

    ParameterHandleValueMap parameterValues;
    std::array<std::uint8_t, 2U> encodedParameter{0x01U, 0x00U};
    parameterValues.emplace(
        parameter,
        VariableLengthData(encodedParameter.data(), encodedParameter.size()));
    std::array<std::uint8_t, 3U> firstTag{0x4FU, 0x4EU, 0x45U};
    std::array<std::uint8_t, 3U> secondTag{0x54U, 0x57U, 0x4FU};
    REQUIRE(senderRti->sendInteraction(
                senderInteraction,
                parameterValues,
                VariableLengthData(firstTag.data(), firstTag.size()),
                rti1516_2025::HLAinteger64Time(5))
                .isValid());
    REQUIRE(senderRti->sendInteraction(
                senderInteraction,
                parameterValues,
                VariableLengthData(secondTag.data(), secondTag.size()),
                rti1516_2025::HLAinteger64Time(8))
                .isValid());
    // The requested Flush Queue frontier is inclusive: a timestamp exactly
    // equal to 10 must cross the process callback boundary before the grant.
    std::array<std::uint8_t, 3U> frontierTag{0x46U, 0x52U, 0x54U};
    REQUIRE(senderRti->sendInteraction(
                senderInteraction,
                parameterValues,
                VariableLengthData(frontierTag.data(), frontierTag.size()),
                rti1516_2025::HLAinteger64Time(10))
                .isValid());
    // A later queued record must remain pending after FQR(10); the process
    // service must not widen admission to the infinite logical-time bound.
    std::array<std::uint8_t, 3U> futureTag{0x46U, 0x55U, 0x54U};
    REQUIRE(senderRti->sendInteraction(
                senderInteraction,
                parameterValues,
                VariableLengthData(futureTag.data(), futureTag.size()),
                rti1516_2025::HLAinteger64Time(12))
                .isValid());
    // A still-future record can be retracted before it crosses the process
    // callback boundary.  The later FQR(20) below proves that this identity
    // is gone rather than merely hidden by the first frontier.
    std::array<std::uint8_t, 3U> retractedFutureTag{0x52U, 0x45U, 0x54U};
    auto const retractedFuture = senderRti->sendInteraction(
        senderInteraction,
        parameterValues,
        VariableLengthData(retractedFutureTag.data(), retractedFutureTag.size()),
        rti1516_2025::HLAinteger64Time(14));
    REQUIRE(retractedFuture.isValid());
    REQUIRE_NOTHROW(senderRti->retract(retractedFuture));
    REQUIRE(receiverFederate.receivedInteractionCount == 0U);
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(8)));
    if (callbackModel == HLA_EVOKED) {
      static_cast<void>(senderRti->evokeCallback(0.0));
    }
    REQUIRE(senderFederate.timeAdvanceGrantCount == 1U);

    REQUIRE_NOTHROW(
        receiverRti->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
    if (callbackModel == HLA_EVOKED) {
      REQUIRE(receiverFederate.receivedInteractionCount == 0U);
      REQUIRE(receiverFederate.flushQueueGrantCount == 0U);
      for (int pass = 0; pass != 12 &&
           (receiverFederate.receivedInteractionCount != 3U ||
                          receiverFederate.flushQueueGrantCount == 0U);
           ++pass) {
        static_cast<void>(receiverRti->evokeCallback(0.0));
      }
    }
    REQUIRE(receiverFederate.receivedInteractionCount == 3U);
    REQUIRE(receiverFederate.flushQueueGrantCount == 1U);
    REQUIRE(
        receiverFederate.callbackOrder == std::vector<char>{'I', 'I', 'I', 'G'});
    REQUIRE(receiverFederate.receivedInteractionTimes ==
            std::vector<std::int64_t>{5, 8, 10});
    REQUIRE(receiverFederate.receivedInteractionClass == expectedClass);
    REQUIRE(receiverFederate.receivedProducingFederate == senderHandle);
    REQUIRE(receiverFederate.receivedParameterCount == 1U);
    REQUIRE(receiverFederate.receivedSentOrder == rti1516_2025::TIMESTAMP);
    REQUIRE(receiverFederate.receivedReceivedOrder == rti1516_2025::TIMESTAMP);
    REQUIRE(receiverFederate.flushQueueGrantImplementation ==
            L"HLAinteger64Time");
    REQUIRE(receiverFederate.flushQueueGrantOptimisticImplementation ==
            L"HLAinteger64Time");
    REQUIRE(receiverFederate.flushQueueGrantValue == 5);
    REQUIRE(receiverFederate.flushQueueGrantOptimisticValue == 5);
    rti1516_2025::HLAinteger64Time queriedTime;
    REQUIRE_NOTHROW(receiverRti->queryLogicalTime(queriedTime));
    REQUIRE(queriedTime.getTime() == 5);

    // The second frontier admits the retained timestamp-12 record.  The
    // exact sequence proves that the timestamp-14 record retracted above did
    // not leave a stale callback behind.
    receiverFederate.callbackOrder.clear();
    REQUIRE_NOTHROW(
        receiverRti->flushQueueRequest(rti1516_2025::HLAinteger64Time(20)));
    if (callbackModel == HLA_EVOKED) {
      REQUIRE(receiverFederate.receivedInteractionCount == 3U);
      REQUIRE(receiverFederate.flushQueueGrantCount == 1U);
      for (int pass = 0; pass != 12 &&
           (receiverFederate.receivedInteractionCount != 4U ||
            receiverFederate.flushQueueGrantCount == 1U);
           ++pass) {
        static_cast<void>(receiverRti->evokeCallback(0.0));
      }
    }
    REQUIRE(receiverFederate.receivedInteractionCount == 4U);
    REQUIRE(receiverFederate.flushQueueGrantCount == 2U);
    REQUIRE(receiverFederate.callbackOrder == std::vector<char>{'I', 'G'});
    REQUIRE(receiverFederate.receivedInteractionTimes ==
            std::vector<std::int64_t>{5, 8, 10, 12});
    REQUIRE(receiverFederate.flushQueueGrantValue == 9);
    REQUIRE(receiverFederate.flushQueueGrantOptimisticValue == 12);
    rti1516_2025::HLAinteger64Time secondQueriedTime;
    REQUIRE_NOTHROW(receiverRti->queryLogicalTime(secondQueriedTime));
    REQUIRE(secondQueriedTime.getTime() == 9);

    REQUIRE_NOTHROW(senderRti->resignFederationExecution(NO_ACTION));
    senderJoined = false;
    REQUIRE_NOTHROW(receiverRti->resignFederationExecution(NO_ACTION));
    receiverJoined = false;
    senderRti->disconnect();
    receiverRti->disconnect();
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
      receiverRti->disconnect();
    } catch (...) {
    }
    try {
      senderRti->disconnect();
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
