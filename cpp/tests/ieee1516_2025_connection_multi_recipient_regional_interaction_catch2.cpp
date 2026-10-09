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
    "RTIambassadors preserve per-recipient regional interaction scope through a configured process endpoint",
    "[integration][foundation][data-distribution-management][interaction-management][callbacks][transport][process-boundary][public-endpoint][multi-federate][regional-interaction][allow-relaxed-ddm][multi-recipient-regional-interaction][rti.service.get-allow-relaxed-ddm-switch][rti.service.get-convey-region-designator-sets-switch][rti.service.set-convey-region-designator-sets-switch][rti.service.subscribe-interaction-class-with-regions][rti.service.unsubscribe-interaction-class-with-regions][rti.service.send-interaction-with-regions][rti.service.create-region][rti.service.set-range-bounds][rti.service.commit-region-modifications][federate.callback.receive-interaction][2025]") {
  auto runScenario = [](CallbackModel callbackModel,
                        bool const relaxedDdmEnabled = false) {
    class RecordingRegionalFederateAmbassador final : public NullFederateAmbassador {
     public:
      struct Delivery final {
        rti1516_2025::InteractionClassHandle interactionClass;
        rti1516_2025::TransportationTypeHandle transportationType;
        rti1516_2025::FederateHandle producingFederate;
        std::size_t parameterCount = 0U;
        bool hasOptionalSentRegions = false;
        std::size_t sentRegionCount = 0U;
        std::vector<std::uint8_t> tag;
      };

      void receiveInteraction(
          rti1516_2025::InteractionClassHandle const& interactionClass,
          rti1516_2025::ParameterHandleValueMap const& parameterValues,
          rti1516_2025::VariableLengthData const& userSuppliedTag,
          rti1516_2025::TransportationTypeHandle const& transportationType,
          rti1516_2025::FederateHandle const& producingFederate,
          rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
        Delivery delivery;
        delivery.interactionClass = interactionClass;
        delivery.transportationType = transportationType;
        delivery.producingFederate = producingFederate;
        delivery.parameterCount = parameterValues.size();
        delivery.hasOptionalSentRegions = optionalSentRegions != nullptr;
        delivery.sentRegionCount = optionalSentRegions == nullptr
            ? 0U
            : optionalSentRegions->size();
        if (userSuppliedTag.size() != 0U) {
          auto const* first =
              static_cast<std::uint8_t const*>(userSuppliedTag.data());
          delivery.tag.assign(first, first + userSuppliedTag.size());
        }
        deliveries.push_back(std::move(delivery));
      }

      std::vector<Delivery> deliveries;
    };

    constexpr wchar_t const* federationName =
        L"public-process-multi-recipient-regional-interaction-execution";
    constexpr wchar_t const* senderName =
        L"public-process-multi-recipient-regional-interaction-sender";
    constexpr wchar_t const* receiverOneName =
        L"public-process-multi-recipient-regional-interaction-receiver-one";
    constexpr wchar_t const* receiverTwoName =
        L"public-process-multi-recipient-regional-interaction-receiver-two";
    constexpr wchar_t const* federateType =
        L"public-process-multi-recipient-regional-interaction-type";
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
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationServiceOptions serviceOptions;
        serviceOptions.pushReceiveOrderEvents = callbackModel == HLA_IMMEDIATE;
        ProcessFederationService service(
            registry,
            composedProcessDefinition(relaxedDdmEnabled),
            serviceOptions);

        auto senderConnection = listener->accept(
            nullptr,
            {"public-process-multi-recipient-regional-interaction-server", 0x9F11U},
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
            "The regional interaction process server lost Create.");
        auto const interactionClass = registry.interactionClassHandleFor(
            federationName, interactionName);
        auto const parameter = registry.parameterHandleFor(
            federationName, interactionName, parameterName);
        auto const dimension = registry.dimensionHandleFor(
            federationName, dimensionName);
        if (!interactionClass || !parameter || !dimension) {
          throw std::runtime_error(
              "The regional interaction process server could not resolve its FOM handles.");
        }
        expectedInteractionClass.store(*interactionClass, std::memory_order_release);
        expectedParameter.store(*parameter, std::memory_order_release);
        expectedDimension.store(*dimension, std::memory_order_release);
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::join_federation_execution,
            "The regional interaction process server lost sender Join.");

        auto receiverOneConnection = listener->accept(
            nullptr,
            {"public-process-multi-recipient-regional-interaction-server", 0x9F12U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession receiverOne(receiverOneConnection);
        auto receiverOneHandler = service.handlerFor(receiverOne);
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::join_federation_execution,
            "The regional interaction process server lost receiver-one Join.");

        auto receiverTwoConnection = listener->accept(
            nullptr,
            {"public-process-multi-recipient-regional-interaction-server", 0x9F13U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession receiverTwo(receiverTwoConnection);
        auto receiverTwoHandler = service.handlerFor(receiverTwo);
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::join_federation_execution,
            "The regional interaction process server lost receiver-two Join.");

          serveExpected(
              sender,
              senderHandler,
              TransportServiceOperation::get_allow_relaxed_ddm_switch,
              "The regional interaction process server lost sender relaxed-DDM lookup.");
          serveExpected(
              receiverOne,
              receiverOneHandler,
              TransportServiceOperation::get_allow_relaxed_ddm_switch,
              "The regional interaction process server lost receiver-one relaxed-DDM lookup.");
          serveExpected(
              receiverTwo,
              receiverTwoHandler,
              TransportServiceOperation::get_allow_relaxed_ddm_switch,
              "The regional interaction process server lost receiver-two relaxed-DDM lookup.");

        // The public clients perform their setup in lockstep across the three
        // sessions (class, parameter, dimension, region, bounds, commit). Keep
        // the scripted process server in that same order so a mismatched
        // request fails immediately instead of leaving a client blocked.
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_interaction_class_handle,
            "The regional interaction process server lost sender class lookup.");
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::get_interaction_class_handle,
            "The regional interaction process server lost receiver-one class lookup.");
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::get_interaction_class_handle,
            "The regional interaction process server lost receiver-two class lookup.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_parameter_handle,
            "The regional interaction process server lost sender parameter lookup.");
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::get_parameter_handle,
            "The regional interaction process server lost receiver-one parameter lookup.");
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::get_parameter_handle,
            "The regional interaction process server lost receiver-two parameter lookup.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_dimension_handle,
            "The regional interaction process server lost sender dimension lookup.");
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::get_dimension_handle,
            "The regional interaction process server lost receiver-one dimension lookup.");
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::get_dimension_handle,
            "The regional interaction process server lost receiver-two dimension lookup.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::create_region,
            "The regional interaction process server lost sender region creation.");
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::create_region,
            "The regional interaction process server lost receiver-one region creation.");
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::create_region,
            "The regional interaction process server lost receiver-two region creation.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::set_range_bounds,
            "The regional interaction process server lost sender bounds.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::commit_region_modifications,
            "The regional interaction process server lost sender region commit.");
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::set_range_bounds,
            "The regional interaction process server lost receiver-one bounds.");
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::commit_region_modifications,
            "The regional interaction process server lost receiver-one region commit.");
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::set_range_bounds,
            "The regional interaction process server lost receiver-two bounds.");
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::commit_region_modifications,
            "The regional interaction process server lost receiver-two region commit.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::publish_interaction_class,
            "The regional interaction process server lost sender Publish.");
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::get_convey_region_designator_sets_switch,
            "The regional interaction process server lost receiver-one Convey lookup.");
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::set_convey_region_designator_sets_switch,
            "The regional interaction process server lost receiver-one Convey set.");
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::get_convey_region_designator_sets_switch,
            "The regional interaction process server lost receiver-one Convey verification.");
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::get_convey_region_designator_sets_switch,
            "The regional interaction process server lost receiver-two Convey lookup.");
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::set_convey_region_designator_sets_switch,
            "The regional interaction process server lost receiver-two Convey set.");
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::get_convey_region_designator_sets_switch,
            "The regional interaction process server lost receiver-two Convey verification.");
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::subscribe_interaction_class_with_regions,
            "The regional interaction process server lost receiver-one regional Subscribe.");
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::subscribe_interaction_class_with_regions,
            "The regional interaction process server lost receiver-two regional Subscribe.");

        auto serveSend = [&](char const* description) {
          if (!umbra::test::servePrimaryProcessRequest(
                  sender, senderHandler,
                  [&](TransportServiceMessage const& request) {
                    if (request.operation !=
                        TransportServiceOperation::send_interaction_with_regions) {
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
          serveExpected(
              sender, senderHandler,
              TransportServiceOperation::report_successful_void_service_invocation,
              "The regional interaction process server lost a successful Send report.");
        };
        serveSend("The regional interaction process server lost first Send.");
        if (relaxedDdmEnabled) {
          auto serveRegionRangeUpdate = [&](ProcessTransportSession& session,
                                            auto const& handler,
                                            char const* recipient) {
            auto const prefix = std::string(
                                    "The regional interaction process server lost ") +
                                recipient + " relaxed-DDM ";
            serveExpected(session, handler,
                          TransportServiceOperation::set_range_bounds,
                          (prefix + "range update.").c_str());
            serveExpected(
                session, handler,
                TransportServiceOperation::commit_region_modifications,
                (prefix + "region commit.").c_str());
          };
          serveRegionRangeUpdate(receiverOne, receiverOneHandler, "receiver-one");
          serveRegionRangeUpdate(receiverTwo, receiverTwoHandler, "receiver-two");
        } else {
          serveExpected(
              sender,
              senderHandler,
              TransportServiceOperation::set_range_bounds,
              "The regional interaction process server lost second-send bounds.");
          serveExpected(
              sender,
              senderHandler,
              TransportServiceOperation::commit_region_modifications,
              "The regional interaction process server lost second-send region commit.");
        }
        serveSend("The regional interaction process server lost second Send.");

        auto serveReceiverEvents = [&](ProcessTransportSession& session,
                                       auto const& handler,
                                       char const* firstDescription) {
          if (callbackModel == HLA_IMMEDIATE) {
            serveExpected(
                session,
                handler,
                TransportServiceOperation::get_interaction_class_handle,
                firstDescription);
          } else {
            serveExpected(
                session,
                handler,
                TransportServiceOperation::receive_interaction,
                firstDescription);
          }
        };
        serveReceiverEvents(
            receiverOne,
            receiverOneHandler,
            "The regional interaction process server lost receiver-one Receive.");
        serveReceiverEvents(
            receiverTwo,
            receiverTwoHandler,
            "The regional interaction process server lost receiver-two Receive.");

        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::unsubscribe_interaction_class_with_regions,
            "The regional interaction process server lost receiver-one Unsubscribe.");
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::unsubscribe_interaction_class_with_regions,
            "The regional interaction process server lost receiver-two Unsubscribe.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::resign_federation_execution,
            "The regional interaction process server lost sender Resign.");
        serveExpected(
            receiverOne,
            receiverOneHandler,
            TransportServiceOperation::resign_federation_execution,
            "The regional interaction process server lost receiver-one Resign.");
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            TransportServiceOperation::resign_federation_execution,
            "The regional interaction process server lost receiver-two Resign.");
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

    RecordingRegionalFederateAmbassador senderFederate;
    RecordingRegionalFederateAmbassador receiverOneFederate;
    RecordingRegionalFederateAmbassador receiverTwoFederate;
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
                  callbackModel,
                  configurationFor(L"public-process-multi-recipient-regional-interaction-sender-client"))
                  .addressUsed);
      senderRti->createFederationExecution(federationName, L"server-owned-fom.xml");
      auto const senderHandle = senderRti->joinFederationExecution(
          senderName, federateType, federationName);
      REQUIRE(senderHandle.isValid());
      senderJoined = true;

      REQUIRE(receiverOneRti->connect(
                  receiverOneFederate,
                  callbackModel,
                  configurationFor(L"public-process-multi-recipient-regional-interaction-receiver-one-client"))
                  .addressUsed);
      auto const receiverOneHandle = receiverOneRti->joinFederationExecution(
          receiverOneName, federateType, federationName);
      REQUIRE(receiverOneHandle.isValid());
      receiverOneJoined = true;

      REQUIRE(receiverTwoRti->connect(
                  receiverTwoFederate,
                  callbackModel,
                  configurationFor(L"public-process-multi-recipient-regional-interaction-receiver-two-client"))
                  .addressUsed);
      auto const receiverTwoHandle = receiverTwoRti->joinFederationExecution(
          receiverTwoName, federateType, federationName);
      REQUIRE(receiverTwoHandle.isValid());
      receiverTwoJoined = true;

      REQUIRE(senderRti->getAllowRelaxedDDMSwitch() == relaxedDdmEnabled);
      REQUIRE(receiverOneRti->getAllowRelaxedDDMSwitch() == relaxedDdmEnabled);
      REQUIRE(receiverTwoRti->getAllowRelaxedDDMSwitch() == relaxedDdmEnabled);

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
      auto const senderParameter = senderRti->getParameterHandle(
          senderInteraction, parameterNameWide);
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
      auto const senderDimension = senderRti->getDimensionHandle(dimensionNameWide);
      auto const receiverOneDimension =
          receiverOneRti->getDimensionHandle(dimensionNameWide);
      auto const receiverTwoDimension =
          receiverTwoRti->getDimensionHandle(dimensionNameWide);
      auto const expectedDimensionText =
          L"DimensionHandle(" +
          std::to_wstring(expectedDimension.load(std::memory_order_acquire)) +
          L")";
      REQUIRE(senderDimension.toString() == expectedDimensionText);
      REQUIRE(receiverOneDimension.toString() == expectedDimensionText);
      REQUIRE(receiverTwoDimension.toString() == expectedDimensionText);

      auto const senderRegion = senderRti->createRegion(
          rti1516_2025::DimensionHandleSet{senderDimension});
      auto const receiverOneRegion = receiverOneRti->createRegion(
          rti1516_2025::DimensionHandleSet{receiverOneDimension});
      auto const receiverTwoRegion = receiverTwoRti->createRegion(
          rti1516_2025::DimensionHandleSet{receiverTwoDimension});
      REQUIRE(senderRegion.isValid());
      REQUIRE(receiverOneRegion.isValid());
      REQUIRE(receiverTwoRegion.isValid());
      REQUIRE_NOTHROW(senderRti->setRangeBounds(
          senderRegion,
          senderDimension,
          relaxedDdmEnabled
              ? rti1516_2025::RangeBounds(0UL, 10UL)
              : rti1516_2025::RangeBounds(0UL, 5UL)));
      REQUIRE_NOTHROW(senderRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{senderRegion}));
      REQUIRE_NOTHROW(receiverOneRti->setRangeBounds(
          receiverOneRegion,
          receiverOneDimension,
          relaxedDdmEnabled
              ? rti1516_2025::RangeBounds(10UL, 20UL)
              : rti1516_2025::RangeBounds(0UL, 5UL)));
      REQUIRE_NOTHROW(receiverOneRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{receiverOneRegion}));
      REQUIRE_NOTHROW(receiverTwoRti->setRangeBounds(
          receiverTwoRegion,
          receiverTwoDimension,
          relaxedDdmEnabled
              ? rti1516_2025::RangeBounds(11UL, 20UL)
              : rti1516_2025::RangeBounds(10UL, 20UL)));
      REQUIRE_NOTHROW(receiverTwoRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{receiverTwoRegion}));

      REQUIRE_NOTHROW(senderRti->publishInteractionClass(senderInteraction));
      REQUIRE_FALSE(receiverOneRti->getConveyRegionDesignatorSetsSwitch());
      REQUIRE_NOTHROW(receiverOneRti->setConveyRegionDesignatorSetsSwitch(true));
      REQUIRE(receiverOneRti->getConveyRegionDesignatorSetsSwitch());
      REQUIRE_FALSE(receiverTwoRti->getConveyRegionDesignatorSetsSwitch());
      REQUIRE_NOTHROW(receiverTwoRti->setConveyRegionDesignatorSetsSwitch(true));
      REQUIRE(receiverTwoRti->getConveyRegionDesignatorSetsSwitch());
      REQUIRE_NOTHROW(receiverOneRti->subscribeInteractionClassWithRegions(
          receiverOneInteraction,
          rti1516_2025::RegionHandleSet{receiverOneRegion},
          true));
      REQUIRE_NOTHROW(receiverTwoRti->subscribeInteractionClassWithRegions(
          receiverTwoInteraction,
          rti1516_2025::RegionHandleSet{receiverTwoRegion},
          true));

      ParameterHandleValueMap parameterValues;
      std::array<std::uint8_t, 2U> encodedParameter{0x01U, 0x00U};
      parameterValues.emplace(
          senderParameter,
          VariableLengthData(encodedParameter.data(), encodedParameter.size()));
      std::array<std::uint8_t, 1U> firstTag{0xA1U};
      REQUIRE_NOTHROW(senderRti->sendInteractionWithRegions(
          senderInteraction,
          parameterValues,
          rti1516_2025::RegionHandleSet{senderRegion},
          VariableLengthData(firstTag.data(), firstTag.size())));
      if (relaxedDdmEnabled) {
        // The first send uses [0, 10): it touches receiver one's [10, 20)
        // region but has a positive gap to receiver two's [11, 20) region.
        // Move the recipients so receiver one has a positive gap and only
        // receiver two touches the unchanged source; gaps remain ineligible.
        REQUIRE_NOTHROW(receiverOneRti->setRangeBounds(
            receiverOneRegion,
            receiverOneDimension,
            rti1516_2025::RangeBounds(11UL, 20UL)));
        REQUIRE_NOTHROW(receiverOneRti->commitRegionModifications(
            rti1516_2025::RegionHandleSet{receiverOneRegion}));
        REQUIRE_NOTHROW(receiverTwoRti->setRangeBounds(
            receiverTwoRegion,
            receiverTwoDimension,
            rti1516_2025::RangeBounds(10UL, 20UL)));
        REQUIRE_NOTHROW(receiverTwoRti->commitRegionModifications(
            rti1516_2025::RegionHandleSet{receiverTwoRegion}));
      }
      if (!relaxedDdmEnabled) {
        REQUIRE_NOTHROW(senderRti->setRangeBounds(
            senderRegion,
            senderDimension,
            rti1516_2025::RangeBounds(10UL, 20UL)));
        REQUIRE_NOTHROW(senderRti->commitRegionModifications(
            rti1516_2025::RegionHandleSet{senderRegion}));
      }
      std::array<std::uint8_t, 1U> secondTag{0xA2U};
      REQUIRE_NOTHROW(senderRti->sendInteractionWithRegions(
          senderInteraction,
          parameterValues,
          rti1516_2025::RegionHandleSet{senderRegion},
          VariableLengthData(secondTag.data(), secondTag.size())));

      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(receiverOneRti->getInteractionClassHandle(interactionNameWide));
        static_cast<void>(receiverTwoRti->getInteractionClassHandle(interactionNameWide));
      } else {
        static_cast<void>(receiverOneRti->evokeCallback(0.0));
        static_cast<void>(receiverTwoRti->evokeCallback(0.0));
      }

      REQUIRE(senderFederate.deliveries.empty());
      REQUIRE(receiverOneFederate.deliveries.size() == 1U);
      REQUIRE(receiverTwoFederate.deliveries.size() == 1U);
      REQUIRE(receiverOneFederate.deliveries.front().interactionClass == expectedClass);
      REQUIRE(receiverTwoFederate.deliveries.front().interactionClass == expectedClass);
      REQUIRE(receiverOneFederate.deliveries.front().transportationType.isValid());
      REQUIRE(receiverTwoFederate.deliveries.front().transportationType.isValid());
      REQUIRE(receiverOneFederate.deliveries.front().producingFederate == senderHandle);
      REQUIRE(receiverTwoFederate.deliveries.front().producingFederate == senderHandle);
      REQUIRE(receiverOneFederate.deliveries.front().parameterCount == 1U);
      REQUIRE(receiverTwoFederate.deliveries.front().parameterCount == 1U);
      REQUIRE(receiverOneFederate.deliveries.front().hasOptionalSentRegions);
      REQUIRE(receiverTwoFederate.deliveries.front().hasOptionalSentRegions);
      REQUIRE(receiverOneFederate.deliveries.front().sentRegionCount == 1U);
      REQUIRE(receiverTwoFederate.deliveries.front().sentRegionCount == 1U);
      REQUIRE(receiverOneFederate.deliveries.front().tag ==
              std::vector<std::uint8_t>{0xA1U});
      REQUIRE(receiverTwoFederate.deliveries.front().tag ==
              std::vector<std::uint8_t>{0xA2U});
      REQUIRE(recipientCount.load(std::memory_order_acquire) == 2U);

      REQUIRE_NOTHROW(receiverOneRti->unsubscribeInteractionClassWithRegions(
          receiverOneInteraction,
          rti1516_2025::RegionHandleSet{receiverOneRegion}));
      REQUIRE_NOTHROW(receiverTwoRti->unsubscribeInteractionClassWithRegions(
          receiverTwoInteraction,
          rti1516_2025::RegionHandleSet{receiverTwoRegion}));
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
  SECTION("HLA_EVOKED with the FDD enabling Relaxed DDM") {
    runScenario(HLA_EVOKED, true);
  }
  SECTION("HLA_IMMEDIATE with the FDD enabling Relaxed DDM") {
    runScenario(HLA_IMMEDIATE, true);
  }
}

#endif
#endif
