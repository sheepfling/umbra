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
    "RTIambassadors preserve per-recipient interaction FIFO through a configured process endpoint",
    "[integration][foundation][interaction-management][callbacks][transport][process-boundary][public-endpoint][multi-federate][callback-ordering][process-multi-recipient-callback-ordering][rti.service.send-interaction][rti.service.receive-interaction][rti.service.evoke-callback][federate.callback.receive-interaction]") {
  auto runScenario = [](CallbackModel callbackModel) {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void receiveInteraction(
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
      ++receivedCount;
      interactionClassHandle = interactionClass;
      transportationTypeHandle = transportationType;
      producingFederateHandle = producingFederate;
      parameterCounts.push_back(parameterValues.size());
      optionalRegions.push_back(optionalSentRegions != nullptr);
      std::vector<std::uint8_t> tag;
      if (userSuppliedTag.size() != 0U) {
        auto const* first = static_cast<std::uint8_t const*>(userSuppliedTag.data());
        tag.assign(first, first + userSuppliedTag.size());
      }
      tags.push_back(std::move(tag));
    }

    std::size_t receivedCount = 0U;
    rti1516_2025::InteractionClassHandle interactionClassHandle;
    rti1516_2025::TransportationTypeHandle transportationTypeHandle;
    rti1516_2025::FederateHandle producingFederateHandle;
    std::vector<std::size_t> parameterCounts;
    std::vector<bool> optionalRegions;
    std::vector<std::vector<std::uint8_t>> tags;
  };

  constexpr wchar_t const* federationName =
      L"public-process-multi-recipient-ordering-execution";
  constexpr wchar_t const* senderName =
      L"public-process-multi-recipient-ordering-sender";
  constexpr wchar_t const* receiverOneName =
      L"public-process-multi-recipient-ordering-receiver-one";
  constexpr wchar_t const* receiverTwoName =
      L"public-process-multi-recipient-ordering-receiver-two";
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
  std::atomic_uint64_t recipientCount{0U};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions serviceOptions;
      serviceOptions.pushReceiveOrderEvents = callbackModel == HLA_IMMEDIATE;
      ProcessFederationService service(
          registry, composedProcessDefinition(), serviceOptions);

      auto senderConnection = listener->accept(
          nullptr,
          {"public-process-multi-recipient-ordering-server", 0x9E01U},
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
          "The multi-recipient process server lost Create.");
      auto const interactionClass = registry.interactionClassHandleFor(
          federationName, interactionName);
      auto const parameter = registry.parameterHandleFor(
          federationName, interactionName, parameterName);
      if (!interactionClass || !parameter) {
        throw std::runtime_error(
            "The multi-recipient process server could not resolve its FOM handles.");
      }
      expectedInteractionClass.store(*interactionClass, std::memory_order_release);
      expectedParameter.store(*parameter, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The multi-recipient process server lost sender Join.");

      auto receiverOneConnection = listener->accept(
          nullptr,
          {"public-process-multi-recipient-ordering-server", 0x9E02U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiverOne(receiverOneConnection);
      auto receiverOneHandler = service.handlerFor(receiverOne);
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::join_federation_execution,
          "The multi-recipient process server lost receiver-one Join.");

      auto receiverTwoConnection = listener->accept(
          nullptr,
          {"public-process-multi-recipient-ordering-server", 0x9E03U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiverTwo(receiverTwoConnection);
      auto receiverTwoHandler = service.handlerFor(receiverTwo);
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::join_federation_execution,
          "The multi-recipient process server lost receiver-two Join.");

      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The multi-recipient process server lost sender lookup.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The multi-recipient process server lost receiver-one lookup.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The multi-recipient process server lost receiver-two lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_parameter_handle,
          "The multi-recipient process server lost parameter lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_interaction_class,
          "The multi-recipient process server lost Publish.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::subscribe_interaction_class,
          "The multi-recipient process server lost receiver-one Subscribe.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::subscribe_interaction_class,
          "The multi-recipient process server lost receiver-two Subscribe.");

      auto serveSend = [&](char const* description) {
        if (!umbra::test::servePrimaryProcessRequest(
                sender, senderHandler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != TransportServiceOperation::send_interaction) {
                    throw std::runtime_error(description);
                  }
                  auto response = senderHandler(request);
                  if (response.status != TransportServiceStatus::ok) {
                    throw std::runtime_error(description);
                  }
                  auto const result =
                      umbra::detail::decodeProcessFederationSendInteractionResult(
                          response.payload);
                  recipientCount.fetch_add(
                      result.recipientCount, std::memory_order_release);
                  return response;
                })) {
          throw std::runtime_error(description);
        }
        umbra::test::serveExpectedSuccessfulVoidInvocationReport(
            serveExpected, sender, senderHandler);
      };
      serveSend("The multi-recipient process server lost first Send.");
      serveSend("The multi-recipient process server lost second Send.");

      auto serveReceiverEvents = [&](ProcessTransportSession& session,
                                     auto const& handler,
                                     char const* firstDescription,
                                     char const* secondDescription) {
        auto const operation = callbackModel == HLA_IMMEDIATE
            ? TransportServiceOperation::get_interaction_class_handle
            : TransportServiceOperation::receive_interaction;
        serveExpected(session, handler, operation, firstDescription);
        if (callbackModel == HLA_EVOKED) {
          serveExpected(session, handler, operation, secondDescription);
        }
      };
      serveReceiverEvents(
          receiverOne,
          receiverOneHandler,
          "The multi-recipient process server lost receiver-one first Receive.",
          "The multi-recipient process server lost receiver-one second Receive.");
      serveReceiverEvents(
          receiverTwo,
          receiverTwoHandler,
          "The multi-recipient process server lost receiver-two first Receive.",
          "The multi-recipient process server lost receiver-two second Receive.");

      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The multi-recipient process server lost sender Resign.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::resign_federation_execution,
          "The multi-recipient process server lost receiver-one Resign.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::resign_federation_execution,
          "The multi-recipient process server lost receiver-two Resign.");
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
                                     L"public-process-multi-recipient-ordering-sender-client")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto receiverOneConfiguration = RtiConfiguration::createConfiguration()
                                      .withConfigurationName(
                                          L"public-process-multi-recipient-ordering-receiver-one-client")
                                      .withRtiAddress(
                                          L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto receiverTwoConfiguration = RtiConfiguration::createConfiguration()
                                      .withConfigurationName(
                                          L"public-process-multi-recipient-ordering-receiver-two-client")
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
    senderRti->createFederationExecution(federationName, L"server-owned-fom.xml");
    auto const senderHandle = senderRti->joinFederationExecution(
        senderName,
        L"public-process-multi-recipient-ordering-type",
        federationName);
    REQUIRE(senderHandle.isValid());
    senderJoined = true;

    REQUIRE(receiverOneRti->connect(
                receiverOneFederate, callbackModel, receiverOneConfiguration)
                .addressUsed);
    auto const receiverOneHandle = receiverOneRti->joinFederationExecution(
        receiverOneName,
        L"public-process-multi-recipient-ordering-type",
        federationName);
    REQUIRE(receiverOneHandle.isValid());
    receiverOneJoined = true;

    REQUIRE(receiverTwoRti->connect(
                receiverTwoFederate, callbackModel, receiverTwoConfiguration)
                .addressUsed);
    auto const receiverTwoHandle = receiverTwoRti->joinFederationExecution(
        receiverTwoName,
        L"public-process-multi-recipient-ordering-type",
        federationName);
    REQUIRE(receiverTwoHandle.isValid());
    receiverTwoJoined = true;

    auto const senderInteraction =
        senderRti->getInteractionClassHandle(interactionNameWide);
    auto const receiverOneInteraction =
        receiverOneRti->getInteractionClassHandle(interactionNameWide);
    auto const receiverTwoInteraction =
        receiverTwoRti->getInteractionClassHandle(interactionNameWide);
    auto const expectedClass =
        rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
            expectedInteractionClass.load(std::memory_order_acquire));
    REQUIRE(senderInteraction == expectedClass);
    REQUIRE(receiverOneInteraction == expectedClass);
    REQUIRE(receiverTwoInteraction == expectedClass);
    auto const parameter = senderRti->getParameterHandle(
        senderInteraction, parameterNameWide);
    REQUIRE(parameter ==
            rti1516_2025::umbra_binding_detail::makeParameterHandle(
                expectedParameter.load(std::memory_order_acquire)));
    REQUIRE_NOTHROW(senderRti->publishInteractionClass(senderInteraction));
    REQUIRE_NOTHROW(
        receiverOneRti->subscribeInteractionClass(receiverOneInteraction, true));
    REQUIRE_NOTHROW(
        receiverTwoRti->subscribeInteractionClass(receiverTwoInteraction, true));

    std::array<std::uint8_t, 2U> encodedParameter{0x01U, 0x00U};
    ParameterHandleValueMap parameterValues;
    parameterValues.emplace(
        parameter,
        VariableLengthData(encodedParameter.data(), encodedParameter.size()));
    std::array<std::uint8_t, 1U> firstTag{0x01U};
    VariableLengthData firstUserSuppliedTag(firstTag.data(), firstTag.size());
    REQUIRE_NOTHROW(senderRti->sendInteraction(
        senderInteraction, parameterValues, firstUserSuppliedTag));
    std::array<std::uint8_t, 1U> secondTag{0x02U};
    VariableLengthData secondUserSuppliedTag(secondTag.data(), secondTag.size());
    REQUIRE_NOTHROW(senderRti->sendInteraction(
        senderInteraction, parameterValues, secondUserSuppliedTag));

    if (callbackModel == HLA_IMMEDIATE) {
      // The pushed-event process path drains all frames captured by the next
      // public service request before that request returns.  Use a harmless
      // official lookup as the polling fence and verify both callbacks were
      // delivered synchronously for each recipient.
      static_cast<void>(receiverOneRti->getInteractionClassHandle(interactionNameWide));
      static_cast<void>(receiverTwoRti->getInteractionClassHandle(interactionNameWide));
    } else {
      static_cast<void>(receiverOneRti->evokeCallback(0.0));
      REQUIRE(receiverOneFederate.receivedCount == 1U);
      REQUIRE(receiverOneFederate.tags[0] == std::vector<std::uint8_t>{0x01U});
      REQUIRE(receiverTwoFederate.receivedCount == 0U);
      static_cast<void>(receiverOneRti->evokeCallback(0.0));
      static_cast<void>(receiverTwoRti->evokeCallback(0.0));
      REQUIRE(receiverTwoFederate.receivedCount == 1U);
      REQUIRE(receiverTwoFederate.tags[0] == std::vector<std::uint8_t>{0x01U});
      static_cast<void>(receiverTwoRti->evokeCallback(0.0));
    }
    REQUIRE(receiverOneFederate.receivedCount == 2U);
    REQUIRE(receiverOneFederate.tags ==
            std::vector<std::vector<std::uint8_t>>{{0x01U}, {0x02U}});
    REQUIRE(receiverTwoFederate.receivedCount == 2U);
    REQUIRE(receiverTwoFederate.tags ==
            std::vector<std::vector<std::uint8_t>>{{0x01U}, {0x02U}});

    for (auto const* receiver : {&receiverOneFederate, &receiverTwoFederate}) {
      REQUIRE(receiver->interactionClassHandle == expectedClass);
      REQUIRE(receiver->transportationTypeHandle.isValid());
      REQUIRE(receiver->producingFederateHandle == senderHandle);
      REQUIRE(receiver->parameterCounts == std::vector<std::size_t>{1U, 1U});
      REQUIRE(receiver->optionalRegions == std::vector<bool>{false, false});
    }
    REQUIRE(senderFederate.receivedCount == 0U);
    REQUIRE(recipientCount.load(std::memory_order_acquire) == 4U);

    senderRti->resignFederationExecution(NO_ACTION);
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
        senderRti->resignFederationExecution(NO_ACTION);
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
