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
    "RTIambassadors fan out multiple timestamped process interactions FIFO to every subscribed receiver before one grant",
    "[integration][foundation][interaction-management][time-management][time-advance]"
    "[transport][process-boundary][public-endpoint][multi-federate][tso]"
    "[multiple-message-ordering][multi-recipient][process-tso-interaction-fanout]"
    "[distinct-timestamp-ordering]"
    "[callback-gating][callback-controls]"
    "[rti.service.send-interaction][rti.service.get-interaction-class-handle]"
    "[rti.service.get-parameter-handle][rti.service.publish-interaction-class]"
    "[rti.service.subscribe-interaction-class][rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained][rti.service.time-advance-request]"
    "[rti.service.query-galt][rti.service.query-lits]"
    "[rti.service.acknowledge-tso-delivery]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]"
    "[2025]") {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void receiveInteraction(
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions,
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::OrderType sentOrder,
        rti1516_2025::OrderType receivedOrder,
        rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
      ++receivedInteractionCount;
      callbackOrder.push_back('I');
      receivedInteractionClass = interactionClass;
      receivedTransportationType = transportationType;
      receivedProducingFederate = producingFederate;
      receivedParameterCount = parameterValues.size();
      hasOptionalSentRegions = optionalSentRegions != nullptr;
      hasOptionalRetraction = optionalRetraction != nullptr;
      if (optionalRetraction != nullptr) {
        retractionValidity.push_back(optionalRetraction->isValid());
      }
      sentOrderType = sentOrder;
      receivedOrderType = receivedOrder;
      auto timestampValue = -1LL;
      if (auto const* integerTime =
              dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
        timestampValue = integerTime->getTime();
      }
      receivedTimestampValues.push_back(timestampValue);
      std::vector<std::uint8_t> tag;
      if (userSuppliedTag.size() != 0U) {
        auto const* data =
            static_cast<std::uint8_t const*>(userSuppliedTag.data());
        tag.assign(data, data + userSuppliedTag.size());
      }
      receivedTags.push_back(std::move(tag));
    }

    void timeConstrainedEnabled(rti1516_2025::LogicalTime const&) override {
      ++timeConstrainedEnabledCount;
    }

    void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
      ++timeAdvanceGrantCount;
      callbackOrder.push_back('G');
      timeAdvanceGrantValue = -1;
      if (auto const* integerTime =
              dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
        timeAdvanceGrantValue = integerTime->getTime();
      }
    }

    std::size_t receivedInteractionCount = 0U;
    rti1516_2025::InteractionClassHandle receivedInteractionClass;
    rti1516_2025::TransportationTypeHandle receivedTransportationType;
    rti1516_2025::FederateHandle receivedProducingFederate;
    std::size_t receivedParameterCount = 0U;
    bool hasOptionalSentRegions = false;
    bool hasOptionalRetraction = false;
    rti1516_2025::OrderType sentOrderType = rti1516_2025::RECEIVE;
    rti1516_2025::OrderType receivedOrderType = rti1516_2025::RECEIVE;
    std::vector<std::int64_t> receivedTimestampValues;
    std::vector<std::vector<std::uint8_t>> receivedTags;
    std::vector<bool> retractionValidity;
    std::size_t timeConstrainedEnabledCount = 0U;
    std::size_t timeAdvanceGrantCount = 0U;
    std::int64_t timeAdvanceGrantValue = -1;
    std::vector<char> callbackOrder;
  } receiverOneFederate, receiverTwoFederate;
  NullFederateAmbassador senderFederate;

  constexpr wchar_t const* federationName =
      L"process-timestamped-interaction-fanout-execution";
  constexpr wchar_t const* senderName =
      L"process-timestamped-interaction-fanout-sender";
  constexpr wchar_t const* receiverOneName =
      L"process-timestamped-interaction-fanout-receiver-one";
  constexpr wchar_t const* receiverTwoName =
      L"process-timestamped-interaction-fanout-receiver-two";
  constexpr wchar_t const* federateType =
      L"process-timestamped-interaction-fanout-type";
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
  std::shared_ptr<ProcessTransportConnection> serverSenderConnection;
  std::shared_ptr<ProcessTransportConnection> serverReceiverOneConnection;
  std::shared_ptr<ProcessTransportConnection> serverReceiverTwoConnection;
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"process-tso-interaction-fanout-server", 0x9420U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      serverSenderConnection = senderConnection;
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
          "The timestamped fanout process server lost Create.");
      auto const interactionClass = registry.interactionClassHandleFor(
          federationName, interactionName);
      auto const parameter = registry.parameterHandleFor(
          federationName, interactionName, parameterName);
      if (!interactionClass || !parameter) {
        throw std::runtime_error(
            "The timestamped fanout process server could not resolve its FOM handles.");
      }
      expectedInteractionClass.store(*interactionClass, std::memory_order_release);
      expectedParameter.store(*parameter, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The timestamped fanout process server lost sender Join.");

      auto receiverOneConnection = listener->accept(
          nullptr,
          {"process-tso-interaction-fanout-server", 0x9421U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      serverReceiverOneConnection = receiverOneConnection;
      ProcessTransportSession receiverOne(receiverOneConnection);
      auto const receiverOneHandler = service.handlerFor(receiverOne);
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::join_federation_execution,
          "The timestamped fanout process server lost receiver-one Join.");

      auto receiverTwoConnection = listener->accept(
          nullptr,
          {"process-tso-interaction-fanout-server", 0x9422U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      serverReceiverTwoConnection = receiverTwoConnection;
      ProcessTransportSession receiverTwo(receiverTwoConnection);
      auto const receiverTwoHandler = service.handlerFor(receiverTwo);
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::join_federation_execution,
          "The timestamped fanout process server lost receiver-two Join.");

      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The timestamped fanout process server lost sender lookup.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The timestamped fanout process server lost receiver-one lookup.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The timestamped fanout process server lost receiver-two lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_parameter_handle,
          "The timestamped fanout process server lost sender parameter lookup.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::get_parameter_handle,
          "The timestamped fanout process server lost receiver-one parameter lookup.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::get_parameter_handle,
          "The timestamped fanout process server lost receiver-two parameter lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_interaction_class,
          "The timestamped fanout process server lost Publish.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::subscribe_interaction_class,
          "The timestamped fanout process server lost receiver-one Subscribe.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::subscribe_interaction_class,
          "The timestamped fanout process server lost receiver-two Subscribe.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::enable_time_regulation,
          "The timestamped fanout process server lost Enable Time Regulation.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::enable_time_constrained,
          "The timestamped fanout process server lost receiver-one Enable Time Constrained.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::enable_time_constrained,
          "The timestamped fanout process server lost receiver-two Enable Time Constrained.");

      auto serveSend = [&](char const* description) {
        if (!umbra::test::servePrimaryProcessRequest(
                sender, senderHandler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation !=
                      TransportServiceOperation::send_interaction) {
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
      };
      serveSend("The timestamped fanout process server lost first Send Interaction.");
      serveSend("The timestamped fanout process server lost second Send Interaction.");
      serveSend("The timestamped fanout process server lost third Send Interaction.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::time_advance_request,
          "The timestamped fanout process server lost receiver-one TAR.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::time_advance_request,
          "The timestamped fanout process server lost receiver-two TAR.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The timestamped fanout process server lost sender TAR.");
      for (auto const* receiverDescription : {
               "The timestamped fanout process server lost receiver-one GALT.",
               "The timestamped fanout process server lost receiver-one LITS."}) {
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::query_time_bounds,
            receiverDescription);
      }
      for (auto const* receiverDescription : {
               "The timestamped fanout process server lost receiver-two GALT.",
               "The timestamped fanout process server lost receiver-two LITS."}) {
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::query_time_bounds,
            receiverDescription);
      }
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::acknowledge_tso_delivery,
          "The timestamped fanout process server lost receiver-one first TSO acknowledgement.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::acknowledge_tso_delivery,
          "The timestamped fanout process server lost receiver-one second TSO acknowledgement.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::acknowledge_tso_delivery,
          "The timestamped fanout process server lost receiver-two first TSO acknowledgement.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::acknowledge_tso_delivery,
          "The timestamped fanout process server lost receiver-two second TSO acknowledgement.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::time_advance_request,
          "The timestamped fanout process server lost receiver-one second TAR.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::time_advance_request,
          "The timestamped fanout process server lost receiver-two second TAR.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The timestamped fanout process server lost sender second TAR.");
      for (auto const* receiverDescription : {
               "The timestamped fanout process server lost receiver-one second GALT.",
               "The timestamped fanout process server lost receiver-one second LITS."}) {
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::query_time_bounds,
            receiverDescription);
      }
      for (auto const* receiverDescription : {
               "The timestamped fanout process server lost receiver-two second GALT.",
               "The timestamped fanout process server lost receiver-two second LITS."}) {
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::query_time_bounds,
            receiverDescription);
      }
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::acknowledge_tso_delivery,
          "The timestamped fanout process server lost receiver-one third TSO acknowledgement.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::acknowledge_tso_delivery,
          "The timestamped fanout process server lost receiver-two third TSO acknowledgement.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The timestamped fanout process server lost sender Resign.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::resign_federation_execution,
          "The timestamped fanout process server lost receiver-one Resign.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::resign_federation_execution,
          "The timestamped fanout process server lost receiver-two Resign.");
      service.detach(sender);
      service.detach(receiverOne);
      service.detach(receiverTwo);
      senderConnection->close();
      receiverOneConnection->close();
      receiverTwoConnection->close();
    } catch (...) {
      serverError = std::current_exception();
      if (serverSenderConnection) {
        serverSenderConnection->close();
      }
      if (serverReceiverOneConnection) {
        serverReceiverOneConnection->close();
      }
      if (serverReceiverTwoConnection) {
        serverReceiverTwoConnection->close();
      }
    }
  });

  auto senderRti = makeRti();
  auto receiverOneRti = makeRti();
  auto receiverTwoRti = makeRti();
  auto configurationFor = [&](wchar_t const* name) {
    return RtiConfiguration::createConfiguration()
        .withConfigurationName(name)
        .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
  };
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverOneJoined = false;
  bool receiverTwoJoined = false;
  try {
    REQUIRE(senderRti->connect(
                senderFederate,
                HLA_EVOKED,
                configurationFor(L"process-timestamped-interaction-fanout-sender-client"))
                .addressUsed);
    REQUIRE_NOTHROW(senderRti->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    auto const senderHandle = senderRti->joinFederationExecution(
        senderName, federateType, federationName);
    REQUIRE(senderHandle.isValid());
    senderJoined = true;

    REQUIRE(receiverOneRti->connect(
                receiverOneFederate,
                HLA_EVOKED,
                configurationFor(
                    L"process-timestamped-interaction-fanout-receiver-one-client"))
                .addressUsed);
    auto const receiverOneHandle = receiverOneRti->joinFederationExecution(
        receiverOneName, federateType, federationName);
    REQUIRE(receiverOneHandle.isValid());
    receiverOneJoined = true;

    REQUIRE(receiverTwoRti->connect(
                receiverTwoFederate,
                HLA_EVOKED,
                configurationFor(
                    L"process-timestamped-interaction-fanout-receiver-two-client"))
                .addressUsed);
    auto const receiverTwoHandle = receiverTwoRti->joinFederationExecution(
        receiverTwoName, federateType, federationName);
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
    auto const senderParameter =
        senderRti->getParameterHandle(senderInteraction, parameterNameWide);
    auto const receiverOneParameter = receiverOneRti->getParameterHandle(
        receiverOneInteraction, parameterNameWide);
    auto const receiverTwoParameter = receiverTwoRti->getParameterHandle(
        receiverTwoInteraction, parameterNameWide);
    auto const expectedParameterHandle =
        rti1516_2025::umbra_binding_detail::makeParameterHandle(
            expectedParameter.load(std::memory_order_acquire));
    REQUIRE(senderParameter == expectedParameterHandle);
    REQUIRE(receiverOneParameter == expectedParameterHandle);
    REQUIRE(receiverTwoParameter == expectedParameterHandle);
    REQUIRE_NOTHROW(senderRti->publishInteractionClass(senderInteraction));
    REQUIRE_NOTHROW(receiverOneRti->subscribeInteractionClass(
        receiverOneInteraction, true));
    REQUIRE_NOTHROW(receiverTwoRti->subscribeInteractionClass(
        receiverTwoInteraction, true));
    REQUIRE_NOTHROW(senderRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_NOTHROW(receiverOneRti->enableTimeConstrained());
    REQUIRE_NOTHROW(receiverTwoRti->enableTimeConstrained());
    static_cast<void>(senderRti->evokeCallback(0.0));
    static_cast<void>(receiverOneRti->evokeCallback(0.0));
    static_cast<void>(receiverTwoRti->evokeCallback(0.0));
    REQUIRE(receiverOneFederate.timeConstrainedEnabledCount == 1U);
    REQUIRE(receiverTwoFederate.timeConstrainedEnabledCount == 1U);

    std::array<std::uint8_t, 1U> const parameterBytes{0x51U};
    ParameterHandleValueMap parameterValues;
    parameterValues.emplace(
        senderParameter,
        VariableLengthData(parameterBytes.data(), parameterBytes.size()));
    std::array<std::uint8_t, 1U> const firstTagBytes{0xA1U};
    std::array<std::uint8_t, 1U> const secondTagBytes{0xB2U};
    std::array<std::uint8_t, 1U> const thirdTagBytes{0xC3U};
    auto const firstRetraction = senderRti->sendInteraction(
        senderInteraction,
        parameterValues,
        VariableLengthData(firstTagBytes.data(), firstTagBytes.size()),
        rti1516_2025::HLAinteger64Time(5));
    auto const secondRetraction = senderRti->sendInteraction(
        senderInteraction,
        parameterValues,
        VariableLengthData(secondTagBytes.data(), secondTagBytes.size()),
        rti1516_2025::HLAinteger64Time(5));
    auto const thirdRetraction = senderRti->sendInteraction(
        senderInteraction,
        parameterValues,
        VariableLengthData(thirdTagBytes.data(), thirdTagBytes.size()),
        rti1516_2025::HLAinteger64Time(7));
    REQUIRE(firstRetraction.isValid());
    REQUIRE(secondRetraction.isValid());
    REQUIRE(thirdRetraction.isValid());
    REQUIRE(firstRetraction.toString() != secondRetraction.toString());
    REQUIRE(firstRetraction.toString() != thirdRetraction.toString());
    REQUIRE(secondRetraction.toString() != thirdRetraction.toString());
    REQUIRE(recipientCount.load(std::memory_order_acquire) == 6U);

    REQUIRE_NOTHROW(receiverOneRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE_NOTHROW(receiverTwoRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));

    auto verifyBounds = [](RTIambassador& rti, std::int64_t expectedTime) {
      rti1516_2025::HLAinteger64Time galt(99);
      REQUIRE(rti.queryGALT(galt));
      REQUIRE(galt.getTime() == expectedTime);
      rti1516_2025::HLAinteger64Time lits(101);
      REQUIRE(rti.queryLITS(lits));
      REQUIRE(lits.getTime() == expectedTime);
    };
    verifyBounds(*receiverOneRti, 5);
    verifyBounds(*receiverTwoRti, 5);

    auto evokeReceiver = [&](RTIambassador& rti,
                             RecordingFederateAmbassador& federate) {
      REQUIRE(federate.receivedInteractionCount == 0U);
      static_cast<void>(rti.evokeCallback(0.0));
      static_cast<void>(rti.evokeCallback(0.0));
      static_cast<void>(rti.evokeCallback(0.0));
      REQUIRE(federate.receivedInteractionCount == 2U);
      REQUIRE(federate.timeAdvanceGrantCount == 1U);
      REQUIRE(federate.timeAdvanceGrantValue == 5);
      REQUIRE(federate.callbackOrder == std::vector<char>{'I', 'I', 'G'});
      REQUIRE(federate.receivedInteractionClass == expectedClass);
      REQUIRE(federate.receivedTransportationType.isValid());
      REQUIRE(federate.receivedProducingFederate == senderHandle);
      REQUIRE(federate.receivedParameterCount == 1U);
      REQUIRE(federate.receivedTimestampValues ==
              std::vector<std::int64_t>{5, 5});
      REQUIRE(federate.receivedTags ==
              std::vector<std::vector<std::uint8_t>>{{0xA1U}, {0xB2U}});
      REQUIRE_FALSE(federate.hasOptionalSentRegions);
      REQUIRE(federate.hasOptionalRetraction);
      REQUIRE(federate.retractionValidity == std::vector<bool>{true, true});
      REQUIRE(federate.sentOrderType == rti1516_2025::TIMESTAMP);
      REQUIRE(federate.receivedOrderType == rti1516_2025::TIMESTAMP);
    };
    evokeReceiver(*receiverOneRti, receiverOneFederate);
    evokeReceiver(*receiverTwoRti, receiverTwoFederate);

    REQUIRE_NOTHROW(receiverOneRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    // Keep one recipient's callback gate closed while the later timestamp is
    // admitted.  The TSO event and its grant must remain queued until the
    // official callback gate is reopened.
    REQUIRE_NOTHROW(receiverTwoRti->disableCallbacks());
    REQUIRE_NOTHROW(receiverTwoRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    verifyBounds(*receiverOneRti, 7);
    verifyBounds(*receiverTwoRti, 7);

    auto evokeSecondTimestamp = [&](RTIambassador& rti,
                                    RecordingFederateAmbassador& federate,
                                    bool callbacksWereDisabled = false) {
      REQUIRE(federate.receivedInteractionCount == 2U);
      if (callbacksWereDisabled) {
        // Evoke admits the already-pushed TSO event but must not invoke the
        // official callback while the switch is disabled.
        REQUIRE(rti.evokeCallback(0.0));
        REQUIRE(federate.receivedInteractionCount == 2U);
        REQUIRE(federate.timeAdvanceGrantCount == 1U);
        REQUIRE(federate.callbackOrder == std::vector<char>{'I', 'I', 'G'});
        REQUIRE_NOTHROW(rti.enableCallbacks());
      }
      static_cast<void>(rti.evokeCallback(0.0));
      REQUIRE(federate.receivedInteractionCount == 3U);
      REQUIRE(federate.timeAdvanceGrantCount == 1U);
      REQUIRE(federate.callbackOrder == std::vector<char>{'I', 'I', 'G', 'I'});
      static_cast<void>(rti.evokeCallback(0.0));
      REQUIRE(federate.timeAdvanceGrantCount == 2U);
      REQUIRE(federate.timeAdvanceGrantValue == 7);
      REQUIRE(federate.callbackOrder == std::vector<char>{'I', 'I', 'G', 'I', 'G'});
      REQUIRE(federate.receivedInteractionClass == expectedClass);
      REQUIRE(federate.receivedTransportationType.isValid());
      REQUIRE(federate.receivedProducingFederate == senderHandle);
      REQUIRE(federate.receivedParameterCount == 1U);
      REQUIRE(federate.receivedTimestampValues ==
              std::vector<std::int64_t>{5, 5, 7});
      REQUIRE(federate.receivedTags ==
              std::vector<std::vector<std::uint8_t>>{{0xA1U}, {0xB2U}, {0xC3U}});
      REQUIRE_FALSE(federate.hasOptionalSentRegions);
      REQUIRE(federate.hasOptionalRetraction);
      REQUIRE(federate.retractionValidity ==
              std::vector<bool>{true, true, true});
      REQUIRE(federate.sentOrderType == rti1516_2025::TIMESTAMP);
      REQUIRE(federate.receivedOrderType == rti1516_2025::TIMESTAMP);
    };
    evokeSecondTimestamp(*receiverOneRti, receiverOneFederate);
    evokeSecondTimestamp(*receiverTwoRti, receiverTwoFederate, true);

    REQUIRE_NOTHROW(senderRti->resignFederationExecution(NO_ACTION));
    senderJoined = false;
    REQUIRE_NOTHROW(receiverOneRti->resignFederationExecution(NO_ACTION));
    receiverOneJoined = false;
    REQUIRE_NOTHROW(receiverTwoRti->resignFederationExecution(NO_ACTION));
    receiverTwoJoined = false;
    REQUIRE_NOTHROW(senderRti->disconnect());
    REQUIRE_NOTHROW(receiverOneRti->disconnect());
    REQUIRE_NOTHROW(receiverTwoRti->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (receiverTwoJoined) {
      try {
        receiverTwoRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (receiverOneJoined) {
      try {
        receiverOneRti->resignFederationExecution(NO_ACTION);
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
  if (serverError) {
    try {
      std::rethrow_exception(serverError);
    } catch (std::exception const& error) {
      FAIL_CHECK(std::string("timestamped interaction fanout server: ") +
                 error.what());
    } catch (...) {
      FAIL_CHECK("timestamped interaction fanout server failed with an unknown exception");
    }
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
}
#endif
#endif
