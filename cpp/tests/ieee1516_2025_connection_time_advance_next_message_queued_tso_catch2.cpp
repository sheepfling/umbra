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
    "RTIambassadors select queued timestamped process messages for next-message advances",
    "[integration][foundation][interaction-management][time-management][time-advance][next-message-request][next-message-request-available][timestamped-interaction][transport][process-boundary][process-time-advance-next-message-queued-focused][dynamic-future-input][dynamic-future-nmra-input][public-endpoint][multi-federate][callbacks][callback-immediate][rti.service.send-interaction][rti.service.enable-time-regulation][rti.service.enable-time-constrained][rti.service.time-advance-request][rti.service.next-message-request][rti.service.next-message-request-available][rti.service-report][federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void receiveInteraction(
        rti1516_2025::InteractionClassHandle const&,
        rti1516_2025::ParameterHandleValueMap const&,
        rti1516_2025::VariableLengthData const&,
        rti1516_2025::TransportationTypeHandle const&,
        rti1516_2025::FederateHandle const&,
        rti1516_2025::RegionHandleSet const*,
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::OrderType,
        rti1516_2025::OrderType,
        rti1516_2025::MessageRetractionHandle const*) override {
      ++receivedInteractionCount;
      callbackOrder.push_back('I');
      if (auto const* integerTime =
              dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
        receivedInteractionTimes.push_back(integerTime->getTime());
      }
    }

    void timeRegulationEnabled(
        rti1516_2025::LogicalTime const&) override {
      ++timeRegulationEnabledCount;
    }

    void timeConstrainedEnabled(
        rti1516_2025::LogicalTime const&) override {
      ++timeConstrainedEnabledCount;
    }

    void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
      ++timeAdvanceGrantCount;
      callbackOrder.push_back('G');
      if (auto const* integerTime =
              dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
        timeAdvanceGrantTimes.push_back(integerTime->getTime());
      }
    }

    std::size_t receivedInteractionCount = 0U;
    std::vector<std::int64_t> receivedInteractionTimes;
    std::size_t timeRegulationEnabledCount = 0U;
    std::size_t timeConstrainedEnabledCount = 0U;
    std::size_t timeAdvanceGrantCount = 0U;
    std::vector<std::int64_t> timeAdvanceGrantTimes;
    std::vector<char> callbackOrder;
  };

  auto runScenario = [](CallbackModel callbackModel) {
  auto const serviceReportDirectory = temporaryServiceReportDirectory();
  constexpr wchar_t const* federationName =
      L"process-next-message-queued-execution";
  constexpr wchar_t const* senderName =
      L"process-next-message-queued-sender";
  constexpr wchar_t const* receiverName =
      L"process-next-message-queued-receiver";
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
  std::shared_ptr<ProcessTransportConnection> senderConnection;
  std::shared_ptr<ProcessTransportConnection> receiverConnection;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions serviceOptions{
          callbackModel == HLA_IMMEDIATE};
      serviceOptions.serviceReportDirectory = serviceReportDirectory;
      ProcessFederationService service(
          registry,
          composedProcessDefinition(),
          std::move(serviceOptions));
      senderConnection = listener->accept(
          nullptr,
          {"process-next-message-queued-server", 0x9214U},
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
                        std::string(description) + " Expected operation " +
                        std::to_string(static_cast<std::uint16_t>(operation)) +
                        " but received " + std::to_string(
                            static_cast<std::uint16_t>(request.operation)) + ".");
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
          "The queued next-message server lost Create.");
      auto const interactionClass = registry.interactionClassHandleFor(
          federationName, interactionName);
      auto const parameter = registry.parameterHandleFor(
          federationName, interactionName, parameterName);
      if (!interactionClass || !parameter) {
        throw std::runtime_error(
            "The queued next-message server could not resolve its FOM handles.");
      }
      expectedInteractionClass.store(*interactionClass, std::memory_order_release);
      expectedParameter.store(*parameter, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The queued next-message server lost sender Join.");

      receiverConnection = listener->accept(
          nullptr,
          {"process-next-message-queued-server", 0x9215U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      // Immediate callbacks can acknowledge the selected TSO concurrently
      // with the caller's logical-time query; either request may arrive first.
      auto serveImmediateQueryAndAcknowledgement = [&](char const* description) {
        bool queryServed = false;
        bool acknowledgementServed = false;
        for (int operation = 0; operation != 2; ++operation) {
          if (!umbra::test::servePrimaryProcessRequest(
                  receiver,
                  receiverHandler,
                  [&](TransportServiceMessage const& request) {
                    if (request.operation ==
                            TransportServiceOperation::query_logical_time &&
                        !queryServed) {
                      queryServed = true;
                    } else if (
                        request.operation ==
                            TransportServiceOperation::acknowledge_tso_delivery &&
                        !acknowledgementServed) {
                      acknowledgementServed = true;
                    } else {
                      throw std::runtime_error(description);
                    }
                    return receiverHandler(request);
                  })) {
            throw std::runtime_error(description);
          }
        }
        if (!queryServed || !acknowledgementServed) {
          throw std::runtime_error(description);
        }
      };
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The queued next-message server lost receiver Join.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The queued next-message server lost sender interaction lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The queued next-message server lost receiver interaction lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_parameter_handle,
          "The queued next-message server lost parameter lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_interaction_class,
          "The queued next-message server lost Publish.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_interaction_class,
          "The queued next-message server lost Subscribe.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::enable_time_regulation,
          "The queued next-message server lost Enable Time Regulation.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::report_successful_void_service_invocation,
          "The queued next-message server lost the Enable Time Regulation report.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::enable_time_constrained,
          "The queued next-message server lost Enable Time Constrained.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::report_successful_void_service_invocation,
          "The queued next-message server lost the Enable Time Constrained report.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::send_interaction,
          "The queued next-message server lost first Send Interaction.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::send_interaction,
          "The queued next-message server lost second Send Interaction.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::send_interaction,
          "The queued next-message server lost third Send Interaction.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The queued next-message server lost sender TAR.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::report_successful_void_service_invocation,
          "The queued next-message server lost sender TAR report.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::set_service_reporting_switch,
          "The queued next-message server lost the service-reporting switch.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::set_send_service_reports_to_file_switch,
          "The queued next-message server lost the service-report file switch.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::next_message_request,
          "The queued next-message server lost receiver NMR.");
      if (callbackModel == HLA_EVOKED) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::report_successful_void_service_invocation,
            "The queued next-message server lost the successful NMR report.");
      }
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::acknowledge_tso_delivery,
          "The queued next-message server lost first TSO acknowledgement.");
      if (callbackModel == HLA_IMMEDIATE) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::report_successful_void_service_invocation,
            "The queued next-message server lost the successful NMR report.");
      }
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::set_service_reporting_switch,
          "The queued next-message server lost the report switch reset.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::query_logical_time,
          "The queued next-message server lost first Query Logical Time.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::report_successful_service_invocation,
          "The queued next-message server lost first Query Logical Time report.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::set_service_reporting_switch,
          "The queued next-message server lost the report switch re-enable.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::next_message_request_available,
          "The queued next-message server lost receiver NMRA.");
      if (callbackModel == HLA_EVOKED) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::report_successful_void_service_invocation,
            "The queued next-message server lost the successful NMRA report.");
      }
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::acknowledge_tso_delivery,
          "The queued next-message server lost second TSO acknowledgement.");
      if (callbackModel == HLA_IMMEDIATE) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::report_successful_void_service_invocation,
            "The queued next-message server lost the successful NMRA report.");
      }
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::set_service_reporting_switch,
          "The queued next-message server lost the second report switch reset.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::query_logical_time,
          "The queued next-message server lost second Query Logical Time.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::report_successful_service_invocation,
          "The queued next-message server lost second Query Logical Time report.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::next_message_request_available,
          "The queued next-message server lost rejected receiver NMRA.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::report_failed_service_invocation,
          "The queued next-message server lost rejected receiver NMRA report.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::query_logical_time,
          "The queued next-message server lost post-rejection Query Logical Time.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::report_successful_service_invocation,
          "The queued next-message server lost post-rejection Query Logical Time report.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::set_service_reporting_switch,
          "The queued next-message server lost the third-request report switch.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::next_message_request,
          "The queued next-message server lost receiver third NMR.");
      if (callbackModel == HLA_EVOKED) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::report_successful_void_service_invocation,
            "The queued next-message server lost the third NMR report.");
      }
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::acknowledge_tso_delivery,
          "The queued next-message server lost third TSO acknowledgement.");
      if (callbackModel == HLA_IMMEDIATE) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::report_successful_void_service_invocation,
            "The queued next-message server lost the third NMR report.");
      }
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::set_service_reporting_switch,
          "The queued next-message server lost the final report switch reset.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::query_logical_time,
          "The queued next-message server lost third Query Logical Time.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::report_successful_service_invocation,
          "The queued next-message server lost third Query Logical Time report.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::next_message_request,
          "The queued next-message server lost future-input receiver NMR.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::report_successful_void_service_invocation,
          "The queued next-message server lost future-input NMR report.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::send_interaction,
          "The queued next-message server lost the first future timestamped interaction.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::send_interaction,
          "The queued next-message server lost the second future timestamped interaction.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The queued next-message server lost the future-input sender TAR.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::report_successful_void_service_invocation,
          "The queued next-message server lost the future-input sender TAR report.");
      if (callbackModel == HLA_IMMEDIATE) {
        serveImmediateQueryAndAcknowledgement(
            "The queued next-message server lost the immediate future-input query or TSO acknowledgement.");
      } else {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::query_logical_time,
            "The queued next-message server lost future-input Query Logical Time.");
      }
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::report_successful_service_invocation,
          "The queued next-message server lost future-input Query Logical Time report.");
      if (callbackModel == HLA_EVOKED) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::acknowledge_tso_delivery,
            "The queued next-message server lost evoked future-input TSO acknowledgement.");
      }
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::next_message_request_available,
          "The queued next-message server lost future-input receiver NMRA.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::report_successful_void_service_invocation,
          "The queued next-message server lost future-input NMRA report.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The queued next-message server lost the future NMRA sender TAR.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::report_successful_void_service_invocation,
          "The queued next-message server lost the future NMRA sender TAR report.");
      if (callbackModel == HLA_IMMEDIATE) {
        serveImmediateQueryAndAcknowledgement(
            "The queued next-message server lost the immediate future-input NMRA query or TSO acknowledgement.");
      } else {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::query_logical_time,
            "The queued next-message server lost future-input NMRA Query Logical Time.");
      }
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::report_successful_service_invocation,
          "The queued next-message server lost future-input NMRA Query Logical Time report.");
      if (callbackModel == HLA_EVOKED) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::acknowledge_tso_delivery,
            "The queued next-message server lost evoked future-input NMRA TSO acknowledgement.");
      }
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The queued next-message server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The queued next-message server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
      if (senderConnection) {
        senderConnection->close();
      }
      if (receiverConnection) {
        receiverConnection->close();
      }
    }
  });

  RecordingFederateAmbassador senderFederate;
  RecordingFederateAmbassador receiverFederate;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto senderConfiguration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"process-next-message-queued-sender-client")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto receiverConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"process-next-message-queued-receiver-client")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(
                senderFederate, callbackModel, senderConfiguration)
                .addressUsed);
    REQUIRE_NOTHROW(senderRti->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    auto const senderHandle = senderRti->joinFederationExecution(
        senderName, L"process-next-message-queued-type", federationName);
    REQUIRE(senderHandle.isValid());
    senderJoined = true;

    REQUIRE(receiverRti->connect(
                receiverFederate, callbackModel, receiverConfiguration)
                .addressUsed);
    auto const receiverHandle = receiverRti->joinFederationExecution(
        receiverName, L"process-next-message-queued-type", federationName);
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
      REQUIRE(senderFederate.timeRegulationEnabledCount == 0U);
      REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    }
    REQUIRE(senderFederate.timeRegulationEnabledCount == 1U);

    REQUIRE_NOTHROW(receiverRti->enableTimeConstrained());
    if (callbackModel == HLA_EVOKED) {
      REQUIRE(receiverFederate.timeConstrainedEnabledCount == 0U);
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.timeConstrainedEnabledCount == 1U);

    auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
        L"HLAinteger64Time");
    auto* integerFactory =
        dynamic_cast<rti1516_2025::HLAinteger64TimeFactory*>(factory.get());
    REQUIRE(integerFactory != nullptr);
    auto timestamp5 = integerFactory->makeLogicalTime(5);
    auto timestamp8 = integerFactory->makeLogicalTime(8);
    auto timestamp12 = integerFactory->makeLogicalTime(12);
    auto timestamp15 = integerFactory->makeLogicalTime(15);
    auto timestamp18 = integerFactory->makeLogicalTime(18);
    REQUIRE(timestamp5);
    REQUIRE(timestamp8);
    REQUIRE(timestamp12);
    REQUIRE(timestamp15);
    REQUIRE(timestamp18);

    std::array<std::uint8_t, 2U> encodedParameter{0x01U, 0x00U};
    ParameterHandleValueMap parameterValues;
    parameterValues.emplace(
        parameter,
        VariableLengthData(encodedParameter.data(), encodedParameter.size()));
    std::array<std::uint8_t, 3U> firstTag{0x4FU, 0x4EU, 0x45U};
    std::array<std::uint8_t, 3U> secondTag{0x54U, 0x57U, 0x4FU};
    std::array<std::uint8_t, 4U> thirdTag{0x54U, 0x48U, 0x52U, 0x45U};
    REQUIRE(senderRti->sendInteraction(
                senderInteraction,
                parameterValues,
                VariableLengthData(firstTag.data(), firstTag.size()),
                *timestamp5)
                .isValid());
    REQUIRE(senderRti->sendInteraction(
                senderInteraction,
                parameterValues,
                VariableLengthData(secondTag.data(), secondTag.size()),
                *timestamp8)
                .isValid());
    REQUIRE(senderRti->sendInteraction(
                senderInteraction,
                parameterValues,
                VariableLengthData(thirdTag.data(), thirdTag.size()),
                *timestamp12)
                .isValid());

    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(12)));
    if (callbackModel == HLA_EVOKED) {
      REQUIRE(senderFederate.timeAdvanceGrantCount == 0U);
      REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    }
    REQUIRE(senderFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(senderFederate.timeAdvanceGrantTimes == std::vector<std::int64_t>{12});

    REQUIRE_NOTHROW(receiverRti->setServiceReportingSwitch(true));
    REQUIRE_NOTHROW(receiverRti->setSendServiceReportsToFileSwitch(true));
    REQUIRE_NOTHROW(receiverRti->nextMessageRequest(
        rti1516_2025::HLAinteger64Time(10)));
    if (callbackModel == HLA_EVOKED) {
      REQUIRE(receiverFederate.receivedInteractionCount == 0U);
      REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.receivedInteractionCount == 1U);
    REQUIRE(receiverFederate.receivedInteractionTimes == std::vector<std::int64_t>{5});
    if (callbackModel == HLA_EVOKED) {
      REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(receiverFederate.timeAdvanceGrantTimes == std::vector<std::int64_t>{5});
    REQUIRE(receiverFederate.callbackOrder == std::vector<char>{'I', 'G'});

    REQUIRE_NOTHROW(receiverRti->setServiceReportingSwitch(false));
    rti1516_2025::HLAinteger64Time afterNmr;
    REQUIRE_NOTHROW(receiverRti->queryLogicalTime(afterNmr));
    REQUIRE(afterNmr.getTime() == 5);
    REQUIRE_NOTHROW(receiverRti->setServiceReportingSwitch(true));

    REQUIRE_NOTHROW(receiverRti->nextMessageRequestAvailable(
        rti1516_2025::HLAinteger64Time(10)));
    if (callbackModel == HLA_EVOKED) {
      REQUIRE(receiverFederate.receivedInteractionCount == 1U);
      REQUIRE(receiverFederate.timeAdvanceGrantCount == 1U);
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.receivedInteractionCount == 2U);
    REQUIRE(receiverFederate.receivedInteractionTimes ==
            std::vector<std::int64_t>{5, 8});
    if (callbackModel == HLA_EVOKED) {
      REQUIRE(receiverFederate.timeAdvanceGrantCount == 1U);
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 2U);
    REQUIRE(receiverFederate.timeAdvanceGrantTimes ==
            std::vector<std::int64_t>{5, 8});
    REQUIRE(receiverFederate.callbackOrder ==
            std::vector<char>{'I', 'G', 'I', 'G'});

    REQUIRE_NOTHROW(receiverRti->setServiceReportingSwitch(false));
    rti1516_2025::HLAinteger64Time afterNmra;
    REQUIRE_NOTHROW(receiverRti->queryLogicalTime(afterNmra));
    REQUIRE(afterNmra.getTime() == 8);
    REQUIRE_THROWS_AS(
        receiverRti->nextMessageRequestAvailable(
            rti1516_2025::HLAinteger64Time(7)),
        rti1516_2025::LogicalTimeAlreadyPassed);
    rti1516_2025::HLAinteger64Time afterRejectedNmra;
    REQUIRE_NOTHROW(receiverRti->queryLogicalTime(afterRejectedNmra));
    REQUIRE(afterRejectedNmra.getTime() == 8);

    REQUIRE_NOTHROW(receiverRti->setServiceReportingSwitch(true));
    REQUIRE_NOTHROW(receiverRti->nextMessageRequest(
        rti1516_2025::HLAinteger64Time(12)));
    if (callbackModel == HLA_EVOKED) {
      REQUIRE(receiverFederate.receivedInteractionCount == 2U);
      REQUIRE(receiverFederate.timeAdvanceGrantCount == 2U);
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.receivedInteractionCount == 3U);
    REQUIRE(receiverFederate.receivedInteractionTimes ==
            std::vector<std::int64_t>{5, 8, 12});
    if (callbackModel == HLA_EVOKED) {
      REQUIRE(receiverFederate.timeAdvanceGrantCount == 2U);
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 3U);
    REQUIRE(receiverFederate.timeAdvanceGrantTimes ==
            std::vector<std::int64_t>{5, 8, 12});
    REQUIRE(receiverFederate.callbackOrder ==
            std::vector<char>{'I', 'G', 'I', 'G', 'I', 'G'});

    REQUIRE_NOTHROW(receiverRti->setServiceReportingSwitch(false));
    rti1516_2025::HLAinteger64Time afterThirdNmr;
    REQUIRE_NOTHROW(receiverRti->queryLogicalTime(afterThirdNmr));
    REQUIRE(afterThirdNmr.getTime() == 12);

    // These inputs cross the process transport only after the NMR has been
    // accepted, and deliberately arrive in reverse timestamp order. The
    // earlier message retargets the pending NMR to 15; the later timestamp
    // must remain queued for a subsequent advance rather than being delivered
    // ahead of the grant at 15.
    REQUIRE_NOTHROW(receiverRti->nextMessageRequest(
        rti1516_2025::HLAinteger64Time(20)));
    REQUIRE(receiverFederate.receivedInteractionCount == 3U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 3U);
    REQUIRE(senderRti->sendInteraction(
                senderInteraction,
                parameterValues,
                VariableLengthData(secondTag.data(), secondTag.size()),
                *timestamp18)
                .isValid());
    REQUIRE(receiverFederate.receivedInteractionCount == 3U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 3U);
    REQUIRE(senderRti->sendInteraction(
                senderInteraction,
                parameterValues,
                VariableLengthData(firstTag.data(), firstTag.size()),
                *timestamp15)
                .isValid());
    REQUIRE(receiverFederate.receivedInteractionCount == 3U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 3U);
    // NMR requires GALT to be strictly greater than the message timestamp;
    // advance the sole regulator past 15 so the pending request can grant at
    // the newly arrived message time of 15.
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(16)));
    if (callbackModel == HLA_EVOKED) {
      REQUIRE(senderFederate.timeAdvanceGrantCount == 1U);
      static_cast<void>(senderRti->evokeCallback(0.0));
    }
    REQUIRE(senderFederate.timeAdvanceGrantCount == 2U);
    REQUIRE(senderFederate.timeAdvanceGrantTimes ==
            std::vector<std::int64_t>{12, 16});
    rti1516_2025::HLAinteger64Time afterFutureNmr;
    REQUIRE_NOTHROW(receiverRti->queryLogicalTime(afterFutureNmr));
    REQUIRE(afterFutureNmr.getTime() == 15);
    if (callbackModel == HLA_EVOKED) {
      for (int attempt = 0;
           attempt != 32 && receiverFederate.receivedInteractionCount < 4U;
           ++attempt) {
        static_cast<void>(receiverRti->evokeCallback(0.01));
      }
      REQUIRE(receiverFederate.receivedInteractionCount == 4U);
      REQUIRE(receiverFederate.timeAdvanceGrantCount == 3U);
      for (int attempt = 0;
           attempt != 32 && receiverFederate.timeAdvanceGrantCount < 4U;
           ++attempt) {
        static_cast<void>(receiverRti->evokeCallback(0.01));
      }
    }
    REQUIRE(receiverFederate.receivedInteractionCount == 4U);
    REQUIRE(receiverFederate.receivedInteractionTimes ==
            std::vector<std::int64_t>{5, 8, 12, 15});
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 4U);
    REQUIRE(receiverFederate.timeAdvanceGrantTimes ==
            std::vector<std::int64_t>{5, 8, 12, 15});
    REQUIRE(receiverFederate.callbackOrder ==
            std::vector<char>{'I', 'G', 'I', 'G', 'I', 'G', 'I', 'G'});

    // NMRA shares the dynamic retarget path but permits equality at GALT.
    // The timestamp-18 message was retained behind the earlier NMR grant;
    // NMRA selects it from the queued process input when the producer reaches
    // 17 and the zero-lookahead GALT reaches 18.
    REQUIRE_NOTHROW(receiverRti->nextMessageRequestAvailable(
        rti1516_2025::HLAinteger64Time(25)));
    REQUIRE(receiverFederate.receivedInteractionCount == 4U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 4U);
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(17)));
    if (callbackModel == HLA_EVOKED) {
      REQUIRE(senderFederate.timeAdvanceGrantCount == 2U);
      static_cast<void>(senderRti->evokeCallback(0.0));
    }
    REQUIRE(senderFederate.timeAdvanceGrantCount == 3U);
    REQUIRE(senderFederate.timeAdvanceGrantTimes ==
            std::vector<std::int64_t>{12, 16, 17});
    rti1516_2025::HLAinteger64Time afterFutureNmra;
    REQUIRE_NOTHROW(receiverRti->queryLogicalTime(afterFutureNmra));
    REQUIRE(afterFutureNmra.getTime() == 18);
    if (callbackModel == HLA_EVOKED) {
      for (int attempt = 0;
           attempt != 32 && receiverFederate.receivedInteractionCount < 5U;
           ++attempt) {
        static_cast<void>(receiverRti->evokeCallback(0.01));
      }
      REQUIRE(receiverFederate.receivedInteractionCount == 5U);
      REQUIRE(receiverFederate.timeAdvanceGrantCount == 4U);
      for (int attempt = 0;
           attempt != 32 && receiverFederate.timeAdvanceGrantCount < 5U;
           ++attempt) {
        static_cast<void>(receiverRti->evokeCallback(0.01));
      }
    }
    REQUIRE(receiverFederate.receivedInteractionCount == 5U);
    REQUIRE(receiverFederate.receivedInteractionTimes ==
            std::vector<std::int64_t>{5, 8, 12, 15, 18});
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 5U);
    REQUIRE(receiverFederate.timeAdvanceGrantTimes ==
            std::vector<std::int64_t>{5, 8, 12, 15, 18});
    REQUIRE(receiverFederate.callbackOrder ==
            std::vector<char>{'I', 'G', 'I', 'G', 'I', 'G', 'I', 'G', 'I', 'G'});

    REQUIRE_NOTHROW(senderRti->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
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
    std::rethrow_exception(serverError);
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(senderJoined);
  REQUIRE_FALSE(receiverJoined);

  std::vector<std::string> reportFiles;
  for (auto const& entry : std::filesystem::directory_iterator(serviceReportDirectory)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    std::ifstream reportFile(entry.path(), std::ios::binary);
    reportFiles.emplace_back(
        std::istreambuf_iterator<char>{reportFile},
        std::istreambuf_iterator<char>{});
  }
  REQUIRE(reportFiles.size() == 2U);
  auto const receiverReport = std::find_if(
      reportFiles.begin(), reportFiles.end(), [](std::string const& report) {
        return report.find(R"("HLAservice":"NextMessageRequest")") !=
            std::string::npos;
      });
  REQUIRE(receiverReport != reportFiles.end());
  auto countOccurrences = [&](std::string const& needle) {
    std::size_t count = 0U;
    std::size_t offset = 0U;
    while ((offset = receiverReport->find(needle, offset)) != std::string::npos) {
      ++count;
      offset += needle.size();
    }
    return count;
  };
  INFO(*receiverReport);
  REQUIRE(countOccurrences(R"("HLAservice":"NextMessageRequest")") == 2U);
  REQUIRE(countOccurrences(
              R"("HLAservice":"NextMessageRequest","HLAsuppliedArguments":[{"HLAargumentType":31,"HLAargumentName":"Logical time","HLAargumentValue":"10"}],"HLAsuccessIndicator":true,"HLAexception":null})") == 1U);
  REQUIRE(countOccurrences(
              R"("HLAservice":"NextMessageRequest","HLAsuppliedArguments":[{"HLAargumentType":31,"HLAargumentName":"Logical time","HLAargumentValue":"12"}],"HLAsuccessIndicator":true,"HLAexception":null})") == 1U);
  REQUIRE(countOccurrences(
              R"("HLAservice":"NextMessageRequestAvailable")") == 1U);
  REQUIRE(countOccurrences(
              R"("HLAservice":"NextMessageRequestAvailable","HLAsuppliedArguments":[{"HLAargumentType":31,"HLAargumentName":"Logical time","HLAargumentValue":"10"}],"HLAsuccessIndicator":true,"HLAexception":null})") == 1U);
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
