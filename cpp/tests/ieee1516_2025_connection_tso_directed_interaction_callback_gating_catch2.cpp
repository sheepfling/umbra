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
    "RTIambassadors retain a timestamped directed interaction while callbacks are disabled",
    "[integration][foundation][federation-management][interaction-management][object-management]"
    "[time-management][time-advance][callbacks][callback-gating][transport]"
    "[process-boundary][public-endpoint][multi-federate][directed-interaction]"
    "[timestamped-directed-interaction][directed-routing]"
    "[process-tso-directed-interaction-callback-gating]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.register-object-instance][rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained][rti.service.time-advance-request]"
    "[rti.service.send-directed-interaction][rti.service.disable-callbacks]"
    "[rti.service.enable-callbacks][rti.service.evoke-callback]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.receive-directed-interaction]"
    "[federate.callback.time-advance-grant][2025]") {
  class RecordingDirectedTsoFederateAmbassador final
      : public NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const& objectClass,
        std::wstring const& objectInstanceName,
        rti1516_2025::FederateHandle const& producingFederate) override {
      ++discoveredCount;
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
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::OrderType sentOrder,
        rti1516_2025::OrderType receivedOrder,
        rti1516_2025::MessageRetractionHandle const* optionalRetraction)
        override {
      ++receivedInteractionCount;
      callbackOrder.push_back('I');
      receivedInteractionClass = interactionClass;
      receivedObjectInstance = objectInstance;
      receivedParameterCount = parameterValues.size();
      receivedTransportationType = transportationType;
      receivedProducingFederate = producingFederate;
      receivedTimestampImplementation = time.implementationName();
      receivedTimestampValue = -1;
      if (auto const* integerTime =
              dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
        receivedTimestampValue = integerTime->getTime();
      }
      receivedSentOrderType = sentOrder;
      receivedReceivedOrderType = receivedOrder;
      hasOptionalRetraction = optionalRetraction != nullptr;
      retractionIsValid = optionalRetraction != nullptr &&
          optionalRetraction->isValid();
      receivedTag.clear();
      if (userSuppliedTag.size() != 0U) {
        auto const* data =
            static_cast<std::uint8_t const*>(userSuppliedTag.data());
        receivedTag.assign(data, data + userSuppliedTag.size());
      }
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

    std::size_t discoveredCount = 0U;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    rti1516_2025::ObjectClassHandle discoveredObjectClass;
    std::wstring discoveredObjectInstanceName;
    rti1516_2025::FederateHandle discoveredProducingFederate;
    std::size_t receivedInteractionCount = 0U;
    rti1516_2025::InteractionClassHandle receivedInteractionClass;
    rti1516_2025::ObjectInstanceHandle receivedObjectInstance;
    std::size_t receivedParameterCount = 0U;
    rti1516_2025::TransportationTypeHandle receivedTransportationType;
    rti1516_2025::FederateHandle receivedProducingFederate;
    std::wstring receivedTimestampImplementation;
    std::int64_t receivedTimestampValue = -1;
    rti1516_2025::OrderType receivedSentOrderType = rti1516_2025::RECEIVE;
    rti1516_2025::OrderType receivedReceivedOrderType = rti1516_2025::RECEIVE;
    bool hasOptionalRetraction = false;
    bool retractionIsValid = false;
    std::vector<std::uint8_t> receivedTag;
    std::size_t timeConstrainedEnabledCount = 0U;
    std::size_t timeAdvanceGrantCount = 0U;
    std::int64_t timeAdvanceGrantValue = -1;
    std::vector<char> callbackOrder;
  } receiverFederate;
  NullFederateAmbassador senderFederate;

  constexpr wchar_t const* federationName =
      L"process-tso-directed-callback-gating-execution";
  constexpr wchar_t const* senderName =
      L"process-tso-directed-callback-gating-sender";
  constexpr wchar_t const* receiverName =
      L"process-tso-directed-callback-gating-receiver";
  constexpr wchar_t const* federateType =
      L"process-tso-directed-callback-gating-type";
  constexpr char const* objectClassNameUtf8 =
      "HLAobjectRoot.UmbraDirectedFixtureObject";
  constexpr wchar_t const* objectClassName =
      L"HLAobjectRoot.UmbraDirectedFixtureObject";
  constexpr char const* attributeNameUtf8 = "DirectedTargetMarker";
  constexpr wchar_t const* attributeName = L"DirectedTargetMarker";
  constexpr char const* interactionClassNameUtf8 =
      "HLAinteractionRoot.UmbraDirectedFixtureInteraction";
  constexpr wchar_t const* interactionClassName =
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction";

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::atomic_uint64_t expectedInteractionClass{0U};
  std::atomic_uint64_t recipientCount{0U};
  std::exception_ptr serverError;
  std::shared_ptr<ProcessTransportConnection> serverSenderConnection;
  std::shared_ptr<ProcessTransportConnection> serverReceiverConnection;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedDirectedProcessDefinition(),
          ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"process-tso-directed-callback-gating-server", 0xA321U},
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
          "The directed callback-gating server lost Create.");
      auto const objectClass =
          registry.objectClassHandleFor(federationName, objectClassNameUtf8);
      auto const attribute = registry.attributeHandleFor(
          federationName, objectClassNameUtf8, attributeNameUtf8);
      auto const interactionClass = registry.interactionClassHandleFor(
          federationName, interactionClassNameUtf8);
      if (!objectClass || !attribute || !interactionClass) {
        throw std::runtime_error(
            "The directed callback-gating server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      expectedInteractionClass.store(
          *interactionClass, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The directed callback-gating server lost sender Join.");

      auto receiverConnection = listener->accept(
          nullptr,
          {"process-tso-directed-callback-gating-server", 0xA322U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      serverReceiverConnection = receiverConnection;
      ProcessTransportSession receiver(receiverConnection);
      auto const receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The directed callback-gating server lost receiver Join.");

      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_object_class_handle,
          "The directed callback-gating server lost sender object lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_object_class_handle,
          "The directed callback-gating server lost receiver object lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_attribute_handle,
          "The directed callback-gating server lost sender attribute lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_attribute_handle,
          "The directed callback-gating server lost receiver attribute lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The directed callback-gating server lost sender interaction lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The directed callback-gating server lost receiver interaction lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The directed callback-gating server lost target publication.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The directed callback-gating server lost target subscription.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_object_class_directed_interactions,
          "The directed callback-gating server lost directed publication.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_directed_interactions,
          "The directed callback-gating server lost directed subscription.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::register_object_instance,
          "The directed callback-gating server lost target registration.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The directed callback-gating server lost discovery poll.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::enable_time_regulation,
          "The directed callback-gating server lost sender time regulation.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::enable_time_constrained,
          "The directed callback-gating server lost receiver time constrained.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::time_advance_request,
          "The directed callback-gating server lost receiver TAR.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The directed callback-gating server lost pre-send poll.");
      if (!umbra::test::servePrimaryProcessRequest(
              sender, senderHandler,
              [&](TransportServiceMessage const& request) {
                if (request.operation !=
                    TransportServiceOperation::send_directed_interaction) {
                  throw std::runtime_error(
                      "The directed callback-gating server lost directed Send.");
                }
                auto response = senderHandler(request);
                if (response.status != TransportServiceStatus::ok) {
                  throw std::runtime_error(
                      "The directed callback-gating server rejected directed Send.");
                }
                auto const result =
                    umbra::detail::decodeProcessFederationSendInteractionResult(
                        response.payload);
                recipientCount.store(
                    result.recipientCount, std::memory_order_release);
                return response;
              })) {
        throw std::runtime_error(
            "The directed callback-gating server lost directed Send.");
      }
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The directed callback-gating server lost sender TAR.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The directed callback-gating server lost post-send poll.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::acknowledge_tso_delivery,
          "The directed callback-gating server lost TSO acknowledgement.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The directed callback-gating server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The directed callback-gating server lost receiver Resign.");
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
  try {
    REQUIRE(senderRti->connect(
                senderFederate,
                HLA_EVOKED,
                configurationFor(
                    L"process-tso-directed-callback-gating-sender-client"))
                .addressUsed);
    REQUIRE_NOTHROW(senderRti->createFederationExecution(
        federationName, L"server-owned-directed-callback-fom.xml"));
    auto const senderHandle = senderRti->joinFederationExecution(
        senderName, federateType, federationName);
    REQUIRE(senderHandle.isValid());
    senderJoined = true;

    REQUIRE(receiverRti->connect(
                receiverFederate,
                HLA_EVOKED,
                configurationFor(
                    L"process-tso-directed-callback-gating-receiver-client"))
                .addressUsed);
    auto const receiverHandle = receiverRti->joinFederationExecution(
        receiverName, federateType, federationName);
    REQUIRE(receiverHandle.isValid());
    receiverJoined = true;

    auto const senderObjectClass =
        senderRti->getObjectClassHandle(objectClassName);
    auto const receiverObjectClass =
        receiverRti->getObjectClassHandle(objectClassName);
    auto const senderAttribute =
        senderRti->getAttributeHandle(senderObjectClass, attributeName);
    auto const receiverAttribute =
        receiverRti->getAttributeHandle(receiverObjectClass, attributeName);
    auto const senderInteraction =
        senderRti->getInteractionClassHandle(interactionClassName);
    auto const receiverInteraction =
        receiverRti->getInteractionClassHandle(interactionClassName);
    REQUIRE(senderObjectClass == receiverObjectClass);
    REQUIRE(senderAttribute == receiverAttribute);
    REQUIRE(senderInteraction == receiverInteraction);
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
        senderObjectClass, rti1516_2025::AttributeHandleSet{senderAttribute}));
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributes(
        receiverObjectClass,
        rti1516_2025::AttributeHandleSet{receiverAttribute},
        true));
    REQUIRE_NOTHROW(senderRti->publishObjectClassDirectedInteractions(
        senderObjectClass,
        rti1516_2025::InteractionClassHandleSet{senderInteraction}));
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassDirectedInteractions(
        receiverObjectClass,
        rti1516_2025::InteractionClassHandleSet{receiverInteraction},
        true));

    auto const objectInstance = senderRti->registerObjectInstance(senderObjectClass);
    REQUIRE(objectInstance.isValid());
    REQUIRE(receiverFederate.discoveredCount == 0U);
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.discoveredCount == 1U);
    REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);
    REQUIRE(receiverFederate.discoveredObjectClass == receiverObjectClass);
    REQUIRE(receiverFederate.discoveredProducingFederate == senderHandle);
    REQUIRE_FALSE(receiverFederate.discoveredObjectInstanceName.empty());

    REQUIRE_NOTHROW(senderRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(0)));
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE_NOTHROW(receiverRti->enableTimeConstrained());
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.timeConstrainedEnabledCount == 1U);
    REQUIRE_NOTHROW(receiverRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.receivedInteractionCount == 0U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);

    REQUIRE_NOTHROW(receiverRti->disableCallbacks());
    std::array<std::uint8_t, 3U> const tagBytes{0x44U, 0x54U, 0x53U};
    auto const retraction = senderRti->sendDirectedInteraction(
        senderInteraction,
        objectInstance,
        rti1516_2025::ParameterHandleValueMap{},
        rti1516_2025::VariableLengthData(tagBytes.data(), tagBytes.size()),
        rti1516_2025::HLAinteger64Time(5));
    REQUIRE(retraction.isValid());
    REQUIRE(recipientCount.load(std::memory_order_acquire) == 1U);
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.receivedInteractionCount == 0U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);
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
    REQUIRE(receiverFederate.receivedInteractionClass == receiverInteraction);
    REQUIRE(receiverFederate.receivedObjectInstance == objectInstance);
    REQUIRE(receiverFederate.receivedParameterCount == 0U);
    REQUIRE(receiverFederate.receivedTransportationType.isValid());
    REQUIRE(receiverFederate.receivedProducingFederate == senderHandle);
    REQUIRE(receiverFederate.receivedTag ==
            std::vector<std::uint8_t>{0x44U, 0x54U, 0x53U});
    REQUIRE(receiverFederate.receivedTimestampImplementation ==
            L"HLAinteger64Time");
    REQUIRE(receiverFederate.receivedTimestampValue == 5);
    REQUIRE(receiverFederate.receivedSentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(receiverFederate.receivedReceivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(receiverFederate.hasOptionalRetraction);
    REQUIRE(receiverFederate.retractionIsValid);
    REQUIRE(receiverFederate.timeAdvanceGrantValue == 5);

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
  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  if (serverError) {
    try {
      std::rethrow_exception(serverError);
    } catch (std::exception const& error) {
      FAIL_CHECK(std::string("directed callback-gating server: ") + error.what());
    } catch (...) {
      FAIL_CHECK("directed callback-gating server failed with an unknown exception");
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
