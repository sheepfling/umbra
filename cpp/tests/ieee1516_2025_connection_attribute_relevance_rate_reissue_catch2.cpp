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
    "RTIambassador reissues Attribute Relevance Advisory callbacks when the active update rate changes through a configured process endpoint",
    "[integration][foundation][object-management][data-distribution-management][callbacks][callback-immediate][transport][process-boundary][public-endpoint][attribute-relevance-advisory][update-rate-reissue][2025][rti.service.get-attribute-relevance-advisory-switch][rti.service.set-attribute-relevance-advisory-switch][rti.service.publish-object-class-attributes][rti.service.register-object-instance][rti.service.subscribe-object-class-attributes][rti.service.unsubscribe-object-class-attributes][federate.callback.discover-object-instance][federate.callback.turn-updates-on-for-object-instance][federate.callback.turn-updates-off-for-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
    class RecordingOwnerAmbassador final : public NullFederateAmbassador {
     public:
      void turnUpdatesOnForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes) override {
        ++turnedOnCount;
        turnedOnObjectInstance = objectInstance;
        turnedOnAttributes = attributes;
        turnedOnRates.emplace_back(std::nullopt);
      }

      void turnUpdatesOnForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes,
          std::wstring const& updateRateDesignator) override {
        ++turnedOnCount;
        turnedOnObjectInstance = objectInstance;
        turnedOnAttributes = attributes;
        turnedOnRates.emplace_back(updateRateDesignator);
      }

      void turnUpdatesOffForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes) override {
        ++turnedOffCount;
        turnedOffObjectInstance = objectInstance;
        turnedOffAttributes = attributes;
      }

      std::size_t turnedOnCount = 0U;
      rti1516_2025::ObjectInstanceHandle turnedOnObjectInstance;
      rti1516_2025::AttributeHandleSet turnedOnAttributes;
      std::vector<std::optional<std::wstring>> turnedOnRates;
      std::size_t turnedOffCount = 0U;
      rti1516_2025::ObjectInstanceHandle turnedOffObjectInstance;
      rti1516_2025::AttributeHandleSet turnedOffAttributes;
    } ownerFederate;

    class RecordingReceiverAmbassador final : public NullFederateAmbassador {
     public:
      void discoverObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::ObjectClassHandle const& objectClass,
          std::wstring const& objectInstanceName,
          rti1516_2025::FederateHandle const& producingFederate) override {
        discovered = true;
        discoveredObjectInstance = objectInstance;
        discoveredObjectClass = objectClass;
        discoveredObjectInstanceName = objectInstanceName;
        discoveredProducingFederate = producingFederate;
      }

      bool discovered = false;
      rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
      rti1516_2025::ObjectClassHandle discoveredObjectClass;
      std::wstring discoveredObjectInstanceName;
      rti1516_2025::FederateHandle discoveredProducingFederate;
    } firstReceiverFederate, secondReceiverFederate;

    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    constexpr wchar_t const* federationName =
        L"public-process-attribute-relevance-rate-reissue-execution";
    constexpr char const* objectClassName = "HLAobjectRoot.Employee";
    constexpr wchar_t const* objectClassNameWide = L"HLAobjectRoot.Employee";
    constexpr char const* nameAttributeName = "Name";
    constexpr wchar_t const* nameAttributeNameWide = L"Name";
    constexpr char const* payRateAttributeName = "PayRate";
    constexpr wchar_t const* payRateAttributeNameWide = L"PayRate";
    std::atomic_uint64_t expectedObjectInstance{0U};
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry,
            composedProcessDefinition(),
            ProcessFederationServiceOptions{true});
        auto ownerConnection = listener->accept(
            nullptr,
            {"public-process-attribute-relevance-rate-reissue-server", 0xA801U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession owner(ownerConnection);
        auto ownerHandler = service.handlerFor(owner);
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
                    if (operation ==
                            TransportServiceOperation::register_object_instance &&
                        response.status == TransportServiceStatus::ok) {
                      auto const registration =
                          umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                              response.payload);
                      expectedObjectInstance.store(
                          registration.objectInstanceHandle,
                          std::memory_order_release);
                    }
                    return response;
                  })) {
            throw std::runtime_error(description);
          }
        };
        auto serveCallbackPoll = [&](ProcessTransportSession& session,
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

        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::create_federation_execution,
            "The rate-reissue server lost Create.");
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::join_federation_execution,
            "The rate-reissue server lost owner Join.");

        auto firstConnection = listener->accept(
            nullptr,
            {"public-process-attribute-relevance-rate-reissue-first", 0xA802U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession firstReceiver(firstConnection);
        auto firstReceiverHandler = service.handlerFor(firstReceiver);
        serveExpected(
            firstReceiver,
            firstReceiverHandler,
            TransportServiceOperation::join_federation_execution,
            "The rate-reissue server lost first receiver Join.");

        auto secondConnection = listener->accept(
            nullptr,
            {"public-process-attribute-relevance-rate-reissue-second", 0xA803U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession secondReceiver(secondConnection);
        auto secondReceiverHandler = service.handlerFor(secondReceiver);
        serveExpected(
            secondReceiver,
            secondReceiverHandler,
            TransportServiceOperation::join_federation_execution,
            "The rate-reissue server lost second receiver Join.");

        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::set_attribute_relevance_advisory_switch,
            "The rate-reissue server lost advisory-switch Set.");
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::get_attribute_relevance_advisory_switch,
            "The rate-reissue server lost advisory-switch Get.");
        for (auto const operation : {
                 TransportServiceOperation::get_object_class_handle,
                 TransportServiceOperation::get_attribute_handle,
                 TransportServiceOperation::get_attribute_handle,
             }) {
          serveExpected(
              owner,
              ownerHandler,
              operation,
              "The rate-reissue server lost owner handle lookup.");
        }
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::publish_object_class_attributes,
            "The rate-reissue server lost Publish.");

        for (auto* receiver : {&firstReceiver, &secondReceiver}) {
          auto const& handler = receiver == &firstReceiver
                                    ? firstReceiverHandler
                                    : secondReceiverHandler;
          serveExpected(
              *receiver,
              handler,
              TransportServiceOperation::get_object_class_handle,
              "The rate-reissue server lost receiver class lookup.");
          serveExpected(
              *receiver,
              handler,
              TransportServiceOperation::get_attribute_handle,
              "The rate-reissue server lost receiver Name lookup.");
          serveExpected(
              *receiver,
              handler,
              TransportServiceOperation::subscribe_object_class_attributes,
              "The rate-reissue server lost initial receiver Subscribe.");
        }

        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::register_object_instance,
            "The rate-reissue server lost Register.");
        serveCallbackPoll(
            firstReceiver,
            firstReceiverHandler,
            "The rate-reissue server lost first discovery poll.");
        serveCallbackPoll(
            secondReceiver,
            secondReceiverHandler,
            "The rate-reissue server lost second discovery poll.");

        serveExpected(
            firstReceiver,
            firstReceiverHandler,
            TransportServiceOperation::get_attribute_handle,
            "The rate-reissue server lost first receiver PayRate lookup.");
        serveExpected(
            firstReceiver,
            firstReceiverHandler,
            TransportServiceOperation::subscribe_object_class_attributes,
            "The rate-reissue server lost first High Subscribe.");
        serveCallbackPoll(
            owner,
            ownerHandler,
            "The rate-reissue server lost first High advisory poll.");

        serveExpected(
            secondReceiver,
            secondReceiverHandler,
            TransportServiceOperation::get_attribute_handle,
            "The rate-reissue server lost second receiver PayRate lookup.");
        serveExpected(
            secondReceiver,
            secondReceiverHandler,
            TransportServiceOperation::subscribe_object_class_attributes,
            "The rate-reissue server lost second Medium Subscribe.");
        serveExpected(
            secondReceiver,
            secondReceiverHandler,
            TransportServiceOperation::subscribe_object_class_attributes,
            "The rate-reissue server lost second Low Subscribe.");

        serveExpected(
            firstReceiver,
            firstReceiverHandler,
            TransportServiceOperation::subscribe_object_class_attributes,
            "The rate-reissue server lost first Medium Subscribe.");
        serveCallbackPoll(
            owner,
            ownerHandler,
            "The rate-reissue server lost Medium advisory poll.");
        serveExpected(
            firstReceiver,
            firstReceiverHandler,
            TransportServiceOperation::subscribe_object_class_attributes,
            "The rate-reissue server lost first High re-subscribe.");
        serveCallbackPoll(
            owner,
            ownerHandler,
            "The rate-reissue server lost High advisory poll.");

        serveExpected(
            firstReceiver,
            firstReceiverHandler,
            TransportServiceOperation::unsubscribe_object_class_attributes,
            "The rate-reissue server lost first PayRate Unsubscribe.");
        serveCallbackPoll(
            owner,
            ownerHandler,
            "The rate-reissue server lost first unsubscribe advisory poll.");
        serveExpected(
            secondReceiver,
            secondReceiverHandler,
            TransportServiceOperation::unsubscribe_object_class_attributes,
            "The rate-reissue server lost second PayRate Unsubscribe.");
        serveCallbackPoll(
            owner,
            ownerHandler,
            "The rate-reissue server lost final advisory-off poll.");

        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::resign_federation_execution,
            "The rate-reissue server lost owner Resign.");
        serveExpected(
            firstReceiver,
            firstReceiverHandler,
            TransportServiceOperation::resign_federation_execution,
            "The rate-reissue server lost first receiver Resign.");
        serveExpected(
            secondReceiver,
            secondReceiverHandler,
            TransportServiceOperation::resign_federation_execution,
            "The rate-reissue server lost second receiver Resign.");
        service.detach(owner);
        service.detach(firstReceiver);
        service.detach(secondReceiver);
        ownerConnection->close();
        firstConnection->close();
        secondConnection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    auto ownerRti = makeRti();
    auto firstReceiverRti = makeRti();
    auto secondReceiverRti = makeRti();
    auto pumpOwner = [&] {
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(ownerRti->getObjectClassHandle(objectClassNameWide));
      } else {
        static_cast<void>(ownerRti->evokeCallback(0.0));
      }
    };
    auto pumpReceiver = [](RTIambassador& rti) {
      static_cast<void>(rti.evokeCallback(0.0));
    };
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-attribute-relevance-rate-reissue-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool ownerJoined = false;
    bool firstReceiverJoined = false;
    bool secondReceiverJoined = false;
    try {
      REQUIRE(ownerRti->connect(ownerFederate, callbackModel, configuration).addressUsed);
      ownerRti->createFederationExecution(federationName, L"server-owned-fom.xml");
      ownerRti->joinFederationExecution(
          L"public-process-attribute-relevance-rate-owner",
          L"public-process-attribute-relevance-rate-owner-type",
          federationName);
      ownerJoined = true;

      REQUIRE(firstReceiverRti->connect(
                  firstReceiverFederate, callbackModel, configuration)
                  .addressUsed);
      firstReceiverRti->joinFederationExecution(
          L"public-process-attribute-relevance-rate-first",
          L"public-process-attribute-relevance-rate-receiver-type",
          federationName);
      firstReceiverJoined = true;

      REQUIRE(secondReceiverRti->connect(
                  secondReceiverFederate, callbackModel, configuration)
                  .addressUsed);
      secondReceiverRti->joinFederationExecution(
          L"public-process-attribute-relevance-rate-second",
          L"public-process-attribute-relevance-rate-receiver-type",
          federationName);
      secondReceiverJoined = true;

      REQUIRE_NOTHROW(ownerRti->setAttributeRelevanceAdvisorySwitch(true));
      REQUIRE(ownerRti->getAttributeRelevanceAdvisorySwitch());

      auto const ownerObjectClass = ownerRti->getObjectClassHandle(objectClassNameWide);
      auto const ownerNameAttribute = ownerRti->getAttributeHandle(
          ownerObjectClass, nameAttributeNameWide);
      auto const ownerPayRateAttribute = ownerRti->getAttributeHandle(
          ownerObjectClass, payRateAttributeNameWide);
      REQUIRE(ownerObjectClass.isValid());
      REQUIRE(ownerNameAttribute.isValid());
      REQUIRE(ownerPayRateAttribute.isValid());
      REQUIRE_NOTHROW(ownerRti->publishObjectClassAttributes(
          ownerObjectClass,
          rti1516_2025::AttributeHandleSet{
              ownerNameAttribute,
              ownerPayRateAttribute}));

      auto const firstReceiverObjectClass =
          firstReceiverRti->getObjectClassHandle(objectClassNameWide);
      auto const firstReceiverNameAttribute = firstReceiverRti->getAttributeHandle(
          firstReceiverObjectClass, nameAttributeNameWide);
      REQUIRE(firstReceiverObjectClass == ownerObjectClass);
      REQUIRE(firstReceiverNameAttribute == ownerNameAttribute);
      REQUIRE_NOTHROW(firstReceiverRti->subscribeObjectClassAttributes(
          firstReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{firstReceiverNameAttribute}));

      auto const secondReceiverObjectClass =
          secondReceiverRti->getObjectClassHandle(objectClassNameWide);
      auto const secondReceiverNameAttribute = secondReceiverRti->getAttributeHandle(
          secondReceiverObjectClass, nameAttributeNameWide);
      REQUIRE(secondReceiverObjectClass == ownerObjectClass);
      REQUIRE(secondReceiverNameAttribute == ownerNameAttribute);
      REQUIRE_NOTHROW(secondReceiverRti->subscribeObjectClassAttributes(
          secondReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{secondReceiverNameAttribute}));

      auto const objectInstance = ownerRti->registerObjectInstance(ownerObjectClass);
      REQUIRE(objectInstance.isValid());
      REQUIRE(objectInstance.toString() ==
              L"ObjectInstanceHandle(" +
                  std::to_wstring(expectedObjectInstance.load(std::memory_order_acquire)) +
                  L")");
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(firstReceiverRti->getObjectClassHandle(objectClassNameWide));
        static_cast<void>(secondReceiverRti->getObjectClassHandle(objectClassNameWide));
      } else {
        pumpReceiver(*firstReceiverRti);
        pumpReceiver(*secondReceiverRti);
      }
      REQUIRE(firstReceiverFederate.discovered);
      REQUIRE(secondReceiverFederate.discovered);
      REQUIRE(firstReceiverFederate.discoveredObjectInstance == objectInstance);
      REQUIRE(secondReceiverFederate.discoveredObjectInstance == objectInstance);
      REQUIRE(firstReceiverFederate.discoveredObjectClass == ownerObjectClass);
      REQUIRE(secondReceiverFederate.discoveredObjectClass == ownerObjectClass);

      // The registration response carries the initial Name advisory. Evoked
      // dispatch retains it locally; immediate dispatch already delivered it.
      if (callbackModel == HLA_EVOKED) {
        pumpOwner();
      }
      REQUIRE(ownerFederate.turnedOnCount == 1U);
      REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOnAttributes ==
              rti1516_2025::AttributeHandleSet{ownerNameAttribute});
      REQUIRE(ownerFederate.turnedOnRates.back() ==
              std::nullopt);
      ownerFederate.turnedOnCount = 0U;
      ownerFederate.turnedOnRates.clear();
      ownerFederate.turnedOnAttributes.clear();

      auto const firstReceiverPayRateAttribute = firstReceiverRti->getAttributeHandle(
          firstReceiverObjectClass, payRateAttributeNameWide);
      REQUIRE(firstReceiverPayRateAttribute == ownerPayRateAttribute);
      REQUIRE_NOTHROW(firstReceiverRti->subscribeObjectClassAttributes(
          firstReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{firstReceiverPayRateAttribute},
          true,
          L"High"));
      pumpOwner();
      REQUIRE(ownerFederate.turnedOnCount == 1U);
      REQUIRE(ownerFederate.turnedOnAttributes ==
              rti1516_2025::AttributeHandleSet{ownerPayRateAttribute});
      REQUIRE(ownerFederate.turnedOnRates.back() ==
              std::optional<std::wstring>{L"High"});

      auto const secondReceiverPayRateAttribute = secondReceiverRti->getAttributeHandle(
          secondReceiverObjectClass, payRateAttributeNameWide);
      REQUIRE(secondReceiverPayRateAttribute == ownerPayRateAttribute);
      REQUIRE_NOTHROW(secondReceiverRti->subscribeObjectClassAttributes(
          secondReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{secondReceiverPayRateAttribute},
          true,
          L"Medium"));
      REQUIRE_NOTHROW(secondReceiverRti->subscribeObjectClassAttributes(
          secondReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{secondReceiverPayRateAttribute},
          true,
          L"Low"));
      REQUIRE(ownerFederate.turnedOnCount == 1U);

      REQUIRE_NOTHROW(firstReceiverRti->subscribeObjectClassAttributes(
          firstReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{firstReceiverPayRateAttribute},
          true,
          L"Medium"));
      pumpOwner();
      REQUIRE(ownerFederate.turnedOnCount == 2U);
      REQUIRE(ownerFederate.turnedOnRates.back() ==
              std::optional<std::wstring>{L"Medium"});

      REQUIRE_NOTHROW(firstReceiverRti->subscribeObjectClassAttributes(
          firstReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{firstReceiverPayRateAttribute},
          true,
          L"High"));
      pumpOwner();
      REQUIRE(ownerFederate.turnedOnCount == 3U);
      REQUIRE(ownerFederate.turnedOnRates.back() ==
              std::optional<std::wstring>{L"High"});

      REQUIRE_NOTHROW(firstReceiverRti->unsubscribeObjectClassAttributes(
          firstReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{firstReceiverPayRateAttribute}));
      pumpOwner();
      REQUIRE(ownerFederate.turnedOffCount == 0U);
      // Removing the higher-rate declaration leaves a still-relevant peer;
      // the boundary carries a Low-rate Turn Updates On refresh, but it must
      // not emit Turn Updates Off while that peer remains subscribed.
      REQUIRE(ownerFederate.turnedOnCount == 4U);
      REQUIRE(ownerFederate.turnedOnRates.back() ==
              std::optional<std::wstring>{L"Low"});

      REQUIRE_NOTHROW(secondReceiverRti->unsubscribeObjectClassAttributes(
          secondReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{secondReceiverPayRateAttribute}));
      pumpOwner();
      REQUIRE(ownerFederate.turnedOffCount == 1U);
      REQUIRE(ownerFederate.turnedOffObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOffAttributes ==
              rti1516_2025::AttributeHandleSet{ownerPayRateAttribute});

      ownerRti->resignFederationExecution(DELETE_OBJECTS);
      ownerJoined = false;
      firstReceiverRti->resignFederationExecution(NO_ACTION);
      firstReceiverJoined = false;
      secondReceiverRti->resignFederationExecution(NO_ACTION);
      secondReceiverJoined = false;
      ownerRti->disconnect();
      firstReceiverRti->disconnect();
      secondReceiverRti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      if (ownerJoined) {
        try {
          ownerRti->resignFederationExecution(DELETE_OBJECTS);
        } catch (...) {
        }
      }
      if (firstReceiverJoined) {
        try {
          firstReceiverRti->resignFederationExecution(NO_ACTION);
        } catch (...) {
        }
      }
      if (secondReceiverJoined) {
        try {
          secondReceiverRti->resignFederationExecution(NO_ACTION);
        } catch (...) {
        }
      }
      try {
        ownerRti->disconnect();
      } catch (...) {
      }
      try {
        firstReceiverRti->disconnect();
      } catch (...) {
      }
      try {
        secondReceiverRti->disconnect();
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
    REQUIRE_FALSE(ownerJoined);
    REQUIRE_FALSE(firstReceiverJoined);
    REQUIRE_FALSE(secondReceiverJoined);
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
