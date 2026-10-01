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
    "RTIambassadors retain a timestamped regional interaction while callbacks are disabled",
    "[integration][foundation][data-distribution-management][interaction-management]"
    "[time-management][time-advance][callbacks][callback-gating][transport]"
    "[process-boundary][public-endpoint][multi-federate][regional-interaction]"
    "[timestamped-process-regional-interaction][regional-ddm-routing]"
    "[process-tso-regional-interaction-callback-gating]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-dimension-handle][rti.service.publish-interaction-class]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.get-convey-region-designator-sets-switch]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][rti.service.disable-callbacks]"
    "[rti.service.enable-callbacks][rti.service.evoke-callback]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]"
    "[2025]") {
  class RecordingRegionalCallbackFederateAmbassador final
      : public NullFederateAmbassador {
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
      sentRegionCount = optionalSentRegions == nullptr
          ? 0U
          : optionalSentRegions->size();
      hasOptionalRetraction = optionalRetraction != nullptr;
      retractionIsValid = optionalRetraction != nullptr &&
          optionalRetraction->isValid();
      sentOrderType = sentOrder;
      receivedOrderType = receivedOrder;
      receivedTimestampImplementation = time.implementationName();
      if (auto const* integerTime =
              dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
        receivedTimestampValue = integerTime->getTime();
      }
      receivedTag.clear();
      if (userSuppliedTag.size() != 0U) {
        auto const* data =
            static_cast<std::uint8_t const*>(userSuppliedTag.data());
        receivedTag.assign(data, data + userSuppliedTag.size());
      }
    }

    void timeConstrainedEnabled(
        rti1516_2025::LogicalTime const&) override {
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
    std::size_t sentRegionCount = 0U;
    bool hasOptionalRetraction = false;
    bool retractionIsValid = false;
    rti1516_2025::OrderType sentOrderType = rti1516_2025::RECEIVE;
    rti1516_2025::OrderType receivedOrderType = rti1516_2025::RECEIVE;
    std::wstring receivedTimestampImplementation;
    std::int64_t receivedTimestampValue = -1;
    std::vector<std::uint8_t> receivedTag;
    std::size_t timeConstrainedEnabledCount = 0U;
    std::size_t timeAdvanceGrantCount = 0U;
    std::int64_t timeAdvanceGrantValue = -1;
    std::vector<char> callbackOrder;
  } receiverFederate;
  NullFederateAmbassador senderFederate;

  constexpr wchar_t const* federationName =
      L"process-tso-regional-callback-gating-execution";
  constexpr wchar_t const* senderName =
      L"process-tso-regional-callback-gating-sender";
  constexpr wchar_t const* receiverName =
      L"process-tso-regional-callback-gating-receiver";
  constexpr wchar_t const* federateType =
      L"process-tso-regional-callback-gating-type";
  constexpr char const* interactionName =
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr wchar_t const* interactionNameWide =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr char const* parameterName = "TimelinessOk";
  constexpr wchar_t const* parameterNameWide = L"TimelinessOk";
  constexpr char const* dimensionName = "ServerId";
  constexpr wchar_t const* dimensionNameWide = L"ServerId";

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  std::atomic_uint64_t expectedInteractionClass{0U};
  std::atomic_uint64_t expectedParameter{0U};
  std::atomic_uint64_t expectedDimension{0U};
  std::atomic_uint64_t recipientCount{0U};
  std::exception_ptr serverError;
  std::shared_ptr<ProcessTransportConnection> serverSenderConnection;
  std::shared_ptr<ProcessTransportConnection> serverReceiverConnection;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"process-tso-regional-callback-gating-server", 0xA311U},
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
          "The regional callback-gating server lost Create.");
      auto const interactionClass = registry.interactionClassHandleFor(
          federationName, interactionName);
      auto const parameter = registry.parameterHandleFor(
          federationName, interactionName, parameterName);
      auto const dimension = registry.dimensionHandleFor(
          federationName, dimensionName);
      if (!interactionClass || !parameter || !dimension) {
        throw std::runtime_error(
            "The regional callback-gating server could not resolve its FOM handles.");
      }
      expectedInteractionClass.store(*interactionClass, std::memory_order_release);
      expectedParameter.store(*parameter, std::memory_order_release);
      expectedDimension.store(*dimension, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The regional callback-gating server lost sender Join.");

      auto receiverConnection = listener->accept(
          nullptr,
          {"process-tso-regional-callback-gating-server", 0xA312U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      serverReceiverConnection = receiverConnection;
      ProcessTransportSession receiver(receiverConnection);
      auto const receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The regional callback-gating server lost receiver Join.");

      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The regional callback-gating server lost sender class lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The regional callback-gating server lost receiver class lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_parameter_handle,
          "The regional callback-gating server lost sender parameter lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_parameter_handle,
          "The regional callback-gating server lost receiver parameter lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_dimension_handle,
          "The regional callback-gating server lost sender dimension lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_dimension_handle,
          "The regional callback-gating server lost receiver dimension lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::create_region,
          "The regional callback-gating server lost sender region creation.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::create_region,
          "The regional callback-gating server lost receiver region creation.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::set_range_bounds,
          "The regional callback-gating server lost sender bounds.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::commit_region_modifications,
          "The regional callback-gating server lost sender region commit.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::set_range_bounds,
          "The regional callback-gating server lost receiver bounds.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::commit_region_modifications,
          "The regional callback-gating server lost receiver region commit.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_interaction_class,
          "The regional callback-gating server lost Publish.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_convey_region_designator_sets_switch,
          "The regional callback-gating server lost Convey lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::set_convey_region_designator_sets_switch,
          "The regional callback-gating server lost Convey set.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_convey_region_designator_sets_switch,
          "The regional callback-gating server lost Convey verification.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_interaction_class_with_regions,
          "The regional callback-gating server lost regional Subscribe.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::enable_time_regulation,
          "The regional callback-gating server lost Enable Time Regulation.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::enable_time_constrained,
          "The regional callback-gating server lost Enable Time Constrained.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::time_advance_request,
          "The regional callback-gating server lost receiver TAR.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The regional callback-gating server lost receiver pre-send poll.");

      if (!umbra::test::servePrimaryProcessRequest(
              sender, senderHandler,
              [&](TransportServiceMessage const& request) {
                if (request.operation !=
                    TransportServiceOperation::send_interaction_with_regions) {
                  throw std::runtime_error(
                      "The regional callback-gating server lost Send Interaction With Regions.");
                }
                auto response = senderHandler(request);
                if (response.status != TransportServiceStatus::ok) {
                  throw std::runtime_error(
                      "The regional callback-gating server rejected Send Interaction With Regions.");
                }
                auto const result =
                    umbra::detail::decodeProcessFederationSendInteractionResult(
                        response.payload);
                recipientCount.store(result.recipientCount, std::memory_order_release);
                return response;
              })) {
        throw std::runtime_error(
            "The regional callback-gating server lost Send Interaction With Regions.");
      }
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The regional callback-gating server lost sender TAR.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The regional callback-gating server lost receiver post-send poll.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::acknowledge_tso_delivery,
          "The regional callback-gating server lost TSO acknowledgement.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::unsubscribe_interaction_class_with_regions,
          "The regional callback-gating server lost regional Unsubscribe.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The regional callback-gating server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The regional callback-gating server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
      if (serverSenderConnection) {
        serverSenderConnection->close();
      }
      if (serverReceiverConnection) {
        serverReceiverConnection->close();
      }
    }
  });

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
  rti1516_2025::FederateHandle senderHandle;
  rti1516_2025::InteractionClassHandle receiverInteraction;
  try {
    REQUIRE(senderRti->connect(
                senderFederate,
                HLA_EVOKED,
                configurationFor(L"process-tso-regional-callback-gating-sender-client"))
                .addressUsed);
    REQUIRE_NOTHROW(senderRti->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    senderHandle = senderRti->joinFederationExecution(
        senderName, federateType, federationName);
    REQUIRE(senderHandle.isValid());
    senderJoined = true;

    REQUIRE(receiverRti->connect(
                receiverFederate,
                HLA_EVOKED,
                configurationFor(L"process-tso-regional-callback-gating-receiver-client"))
                .addressUsed);
    auto const receiverHandle = receiverRti->joinFederationExecution(
        receiverName, federateType, federationName);
    REQUIRE(receiverHandle.isValid());
    receiverJoined = true;

    auto const senderInteraction =
        senderRti->getInteractionClassHandle(interactionNameWide);
    receiverInteraction =
        receiverRti->getInteractionClassHandle(interactionNameWide);
    auto const expectedClass =
        rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
            expectedInteractionClass.load(std::memory_order_acquire));
    REQUIRE(senderInteraction == expectedClass);
    REQUIRE(receiverInteraction == expectedClass);
    auto const senderParameter = senderRti->getParameterHandle(
        senderInteraction, parameterNameWide);
    auto const receiverParameter = receiverRti->getParameterHandle(
        receiverInteraction, parameterNameWide);
    auto const expectedParameterHandle =
        rti1516_2025::umbra_binding_detail::makeParameterHandle(
            expectedParameter.load(std::memory_order_acquire));
    REQUIRE(senderParameter == expectedParameterHandle);
    REQUIRE(receiverParameter == expectedParameterHandle);
    auto const senderDimension = senderRti->getDimensionHandle(dimensionNameWide);
    auto const receiverDimension =
        receiverRti->getDimensionHandle(dimensionNameWide);
    auto const expectedDimensionText =
        L"DimensionHandle(" +
        std::to_wstring(expectedDimension.load(std::memory_order_acquire)) +
        L")";
    REQUIRE(senderDimension.toString() == expectedDimensionText);
    REQUIRE(receiverDimension.toString() == expectedDimensionText);

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
        receiverRegion, receiverDimension, rti1516_2025::RangeBounds(0UL, 5UL)));
    REQUIRE_NOTHROW(receiverRti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{receiverRegion}));
    REQUIRE_NOTHROW(senderRti->publishInteractionClass(senderInteraction));
    REQUIRE_FALSE(receiverRti->getConveyRegionDesignatorSetsSwitch());
    REQUIRE_NOTHROW(receiverRti->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE(receiverRti->getConveyRegionDesignatorSetsSwitch());
    REQUIRE_NOTHROW(receiverRti->subscribeInteractionClassWithRegions(
        receiverInteraction,
        rti1516_2025::RegionHandleSet{receiverRegion},
        true));

    REQUIRE_NOTHROW(senderRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(0)));
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE_NOTHROW(receiverRti->enableTimeConstrained());
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.timeConstrainedEnabledCount == 1U);

    REQUIRE_NOTHROW(receiverRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE(receiverFederate.receivedInteractionCount == 0U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);
    static_cast<void>(receiverRti->evokeCallback(0.0));

    REQUIRE_NOTHROW(receiverRti->disableCallbacks());
    std::array<std::uint8_t, 2U> const parameterBytes{0x01U, 0x00U};
    rti1516_2025::ParameterHandleValueMap parameterValues;
    parameterValues.emplace(
        senderParameter,
        rti1516_2025::VariableLengthData(
            parameterBytes.data(), parameterBytes.size()));
    std::array<std::uint8_t, 3U> const tagBytes{0x52U, 0x45U, 0x47U};
    auto const retraction = senderRti->sendInteractionWithRegions(
        senderInteraction,
        parameterValues,
        rti1516_2025::RegionHandleSet{senderRegion},
        rti1516_2025::VariableLengthData(tagBytes.data(), tagBytes.size()),
        rti1516_2025::HLAinteger64Time(5));
    REQUIRE(retraction.isValid());
    REQUIRE(recipientCount.load(std::memory_order_acquire) == 1U);

    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.receivedInteractionCount == 0U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);
    // The event and its matching grant are already queued at the process
    // boundary, but the official callback switch keeps both out of the
    // federate ambassador until it is explicitly reopened.
    REQUIRE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.receivedInteractionCount == 0U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);

    REQUIRE_NOTHROW(receiverRti->enableCallbacks());
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.receivedInteractionCount == 1U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);
    REQUIRE(receiverFederate.callbackOrder == std::vector<char>{'I'});
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(receiverFederate.callbackOrder == std::vector<char>{'I', 'G'});
    REQUIRE(receiverFederate.receivedInteractionClass == expectedClass);
    REQUIRE(receiverFederate.receivedTransportationType.isValid());
    REQUIRE(receiverFederate.receivedProducingFederate == senderHandle);
    REQUIRE(receiverFederate.receivedParameterCount == 1U);
    REQUIRE(receiverFederate.receivedTag ==
            std::vector<std::uint8_t>{0x52U, 0x45U, 0x47U});
    REQUIRE(receiverFederate.hasOptionalSentRegions);
    REQUIRE(receiverFederate.sentRegionCount == 1U);
    REQUIRE(receiverFederate.hasOptionalRetraction);
    REQUIRE(receiverFederate.retractionIsValid);
    REQUIRE(receiverFederate.receivedTimestampImplementation ==
            L"HLAinteger64Time");
    REQUIRE(receiverFederate.receivedTimestampValue == 5);
    REQUIRE(receiverFederate.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(receiverFederate.receivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(receiverFederate.timeAdvanceGrantValue == 5);

    REQUIRE_NOTHROW(receiverRti->unsubscribeInteractionClassWithRegions(
        receiverInteraction,
        rti1516_2025::RegionHandleSet{receiverRegion}));
    REQUIRE_NOTHROW(senderRti->resignFederationExecution(NO_ACTION));
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
  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  if (serverError) {
    try {
      std::rethrow_exception(serverError);
    } catch (std::exception const& error) {
      FAIL_CHECK(std::string("regional callback-gating server: ") + error.what());
    } catch (...) {
      FAIL_CHECK("regional callback-gating server failed with an unknown exception");
    }
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(senderJoined);
  REQUIRE_FALSE(receiverJoined);
}
#endif
#endif
