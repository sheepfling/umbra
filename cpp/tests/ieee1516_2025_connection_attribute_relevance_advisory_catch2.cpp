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
    "RTIambassador delivers Attribute Relevance Advisory callbacks through a configured process endpoint under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][foundation][object-management][data-distribution-management][callbacks][callback-immediate][transport][process-boundary][public-endpoint][2025][rti.service.get-attribute-relevance-advisory-switch][rti.service.set-attribute-relevance-advisory-switch][rti.service.publish-object-class-attributes][rti.service.register-object-instance][rti.service.subscribe-object-class-attributes][rti.service.unsubscribe-object-class-attributes][federate.callback.turn-updates-on-for-object-instance][federate.callback.turn-updates-off-for-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
  class RecordingAttributeRelevanceFederateAmbassador final
      : public NullFederateAmbassador {
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

    void turnUpdatesOnForObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleSet const& attributes) override {
      turnedOn = true;
      turnedOnObjectInstance = objectInstance;
      turnedOnAttributeCount = attributes.size();
      turnedOnAttributes = attributes;
      turnedOnUpdateRateDesignator.reset();
    }

    void turnUpdatesOnForObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleSet const& attributes,
        std::wstring const& updateRateDesignator) override {
      turnedOn = true;
      turnedOnObjectInstance = objectInstance;
      turnedOnAttributeCount = attributes.size();
      turnedOnAttributes = attributes;
      turnedOnUpdateRateDesignator = updateRateDesignator;
    }

    void turnUpdatesOffForObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleSet const& attributes) override {
      turnedOff = true;
      turnedOffObjectInstance = objectInstance;
      turnedOffAttributeCount = attributes.size();
      turnedOffAttributes = attributes;
    }

    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    rti1516_2025::ObjectClassHandle discoveredObjectClass;
    std::wstring discoveredObjectInstanceName;
    rti1516_2025::FederateHandle discoveredProducingFederate;
    bool turnedOn = false;
    rti1516_2025::ObjectInstanceHandle turnedOnObjectInstance;
    rti1516_2025::AttributeHandleSet turnedOnAttributes;
    std::size_t turnedOnAttributeCount = 0U;
    std::optional<std::wstring> turnedOnUpdateRateDesignator;
    bool turnedOff = false;
    rti1516_2025::ObjectInstanceHandle turnedOffObjectInstance;
    rti1516_2025::AttributeHandleSet turnedOffAttributes;
    std::size_t turnedOffAttributeCount = 0U;
  } ownerFederate, receiverFederate;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-attribute-relevance-advisory-execution";
  constexpr char const* objectClassName = "HLAobjectRoot.Employee";
  constexpr wchar_t const* objectClassNameWide = L"HLAobjectRoot.Employee";
  constexpr char const* nameAttributeName = "Name";
  constexpr wchar_t const* nameAttributeNameWide = L"Name";
  constexpr char const* payRateAttributeName = "PayRate";
  constexpr wchar_t const* payRateAttributeNameWide = L"PayRate";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedNameAttribute{0U};
  std::atomic_uint64_t expectedPayRateAttribute{0U};
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
          {"public-process-attribute-relevance-advisory-server", 0xA501U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession owner(ownerConnection);
      auto ownerHandler = service.handlerFor(owner);
      std::shared_ptr<ProcessTransportConnection> receiverConnection;
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
                  if (operation == TransportServiceOperation::register_object_instance &&
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

      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::create_federation_execution,
          "The public process advisory server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationName, objectClassName);
      auto const nameAttribute = registry.attributeHandleFor(
          federationName, objectClassName, nameAttributeName);
      auto const payRateAttribute = registry.attributeHandleFor(
          federationName, objectClassName, payRateAttributeName);
      if (!objectClass || !nameAttribute || !payRateAttribute) {
        throw std::runtime_error(
            "The public process advisory server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedNameAttribute.store(*nameAttribute, std::memory_order_release);
      expectedPayRateAttribute.store(*payRateAttribute, std::memory_order_release);
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::join_federation_execution,
          "The public process advisory server lost owner Join.");
      receiverConnection = listener->accept(
          nullptr,
          {"public-process-attribute-relevance-advisory-server", 0xA502U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The public process advisory server lost receiver Join.");
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::set_attribute_relevance_advisory_switch,
          "The public process advisory server lost advisory-switch Set.");
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::get_attribute_relevance_advisory_switch,
          "The public process advisory server lost advisory-switch Get.");
      for (auto const operation : {
               TransportServiceOperation::get_object_class_handle,
               TransportServiceOperation::get_attribute_handle,
               TransportServiceOperation::get_attribute_handle,
           }) {
        serveExpected(
            owner,
            ownerHandler,
            operation,
            "The public process advisory server lost owner handle lookup.");
      }
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The public process advisory server lost Publish.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_object_class_handle,
          "The public process advisory server lost receiver class lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_attribute_handle,
          "The public process advisory server lost receiver Name lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The public process advisory server lost initial Subscribe.");
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::register_object_instance,
          "The public process advisory server lost Register.");
      if (callbackModel == HLA_IMMEDIATE) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_object_class_handle,
            "The public process advisory server lost immediate discovery lookup.");
      } else {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::receive_interaction,
            "The public process advisory server lost discovery Receive.");
      }
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_attribute_handle,
          "The public process advisory server lost receiver PayRate lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The public process advisory server lost PayRate Subscribe.");
      if (callbackModel == HLA_IMMEDIATE) {
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::get_object_class_handle,
            "The public process advisory server lost immediate Turn Updates On lookup.");
      } else {
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::receive_interaction,
            "The public process advisory server lost Turn Updates On Receive.");
      }
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::unsubscribe_object_class_attributes,
          "The public process advisory server lost PayRate Unsubscribe.");
      if (callbackModel == HLA_IMMEDIATE) {
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::get_object_class_handle,
            "The public process advisory server lost immediate Turn Updates Off lookup.");
      } else {
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::receive_interaction,
            "The public process advisory server lost Turn Updates Off Receive.");
      }
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public process advisory server lost owner Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public process advisory server lost receiver Resign.");
      service.detach(owner);
      service.detach(receiver);
      ownerConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto ownerRti = makeRti();
  auto receiverRti = makeRti();
  auto pumpEvoked = [&](RTIambassador& rti) {
    if (callbackModel == HLA_EVOKED) {
      static_cast<void>(rti.evokeCallback(0.0));
    }
  };
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"public-process-attribute-relevance-advisory-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool ownerJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(ownerRti->connect(ownerFederate, callbackModel, configuration).addressUsed);
    ownerRti->createFederationExecution(federationName, L"server-owned-fom.xml");
    ownerRti->joinFederationExecution(
        L"public-process-attribute-relevance-owner",
        L"public-process-attribute-relevance-type",
        federationName);
    ownerJoined = true;

    REQUIRE(receiverRti->connect(receiverFederate, callbackModel, configuration).addressUsed);
    receiverRti->joinFederationExecution(
        L"public-process-attribute-relevance-receiver",
        L"public-process-attribute-relevance-type",
        federationName);
    receiverJoined = true;

    REQUIRE_NOTHROW(ownerRti->setAttributeRelevanceAdvisorySwitch(true));
    REQUIRE(ownerRti->getAttributeRelevanceAdvisorySwitch());

    auto const ownerObjectClass = ownerRti->getObjectClassHandle(objectClassNameWide);
    auto const ownerNameAttribute = ownerRti->getAttributeHandle(
        ownerObjectClass, nameAttributeNameWide);
    auto const ownerPayRateAttribute = ownerRti->getAttributeHandle(
        ownerObjectClass, payRateAttributeNameWide);
    REQUIRE(ownerObjectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    REQUIRE(ownerNameAttribute ==
            rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                expectedNameAttribute.load(std::memory_order_acquire)));
    REQUIRE(ownerPayRateAttribute ==
            rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                expectedPayRateAttribute.load(std::memory_order_acquire)));
    REQUIRE_NOTHROW(ownerRti->publishObjectClassAttributes(
        ownerObjectClass,
        rti1516_2025::AttributeHandleSet{
            ownerNameAttribute,
            ownerPayRateAttribute}));

    auto const receiverObjectClass =
        receiverRti->getObjectClassHandle(objectClassNameWide);
    auto const receiverNameAttribute = receiverRti->getAttributeHandle(
        receiverObjectClass, nameAttributeNameWide);
    REQUIRE(receiverObjectClass == ownerObjectClass);
    REQUIRE(receiverNameAttribute == ownerNameAttribute);
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributes(
        receiverObjectClass,
        rti1516_2025::AttributeHandleSet{receiverNameAttribute}));

    auto const objectInstance = ownerRti->registerObjectInstance(ownerObjectClass);
    REQUIRE(objectInstance.isValid());
    REQUIRE(objectInstance.toString() ==
            L"ObjectInstanceHandle(" +
                std::to_wstring(expectedObjectInstance.load(std::memory_order_acquire)) +
                L")");
    if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(receiverRti->getObjectClassHandle(objectClassNameWide));
    } else {
      pumpEvoked(*receiverRti);
    }
    REQUIRE(receiverFederate.discovered);
    REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);
    REQUIRE(receiverFederate.discoveredObjectClass == receiverObjectClass);

    // Registration/discovery can make the receiver's initial declaration
    // relevant immediately. Drain that owner-directed advisory before
    // changing the receiver declaration so the later PayRate transition stays
    // isolated under both callback models.
    if (callbackModel == HLA_EVOKED) {
      pumpEvoked(*ownerRti);
    }
    REQUIRE(ownerFederate.turnedOn);
    REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
    REQUIRE(ownerFederate.turnedOnAttributes.contains(ownerNameAttribute));
    REQUIRE(ownerFederate.turnedOnUpdateRateDesignator == std::nullopt);
    ownerFederate.turnedOn = false;
    ownerFederate.turnedOnObjectInstance = {};
    ownerFederate.turnedOnAttributes.clear();
    ownerFederate.turnedOnAttributeCount = 0U;
    ownerFederate.turnedOnUpdateRateDesignator.reset();

    auto const receiverPayRateAttribute = receiverRti->getAttributeHandle(
        receiverObjectClass, payRateAttributeNameWide);
    REQUIRE(receiverPayRateAttribute == ownerPayRateAttribute);
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributes(
        receiverObjectClass,
        rti1516_2025::AttributeHandleSet{receiverPayRateAttribute},
        true,
        L"High"));

    if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(ownerRti->getObjectClassHandle(objectClassNameWide));
    } else {
      pumpEvoked(*ownerRti);
    }
    REQUIRE(ownerFederate.turnedOn);
    REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
    REQUIRE(ownerFederate.turnedOnAttributeCount == 1U);
    REQUIRE(ownerFederate.turnedOnAttributes.contains(ownerPayRateAttribute));
    REQUIRE(ownerFederate.turnedOnUpdateRateDesignator == std::optional<std::wstring>{L"High"});

    REQUIRE_NOTHROW(receiverRti->unsubscribeObjectClassAttributes(
        receiverObjectClass,
        rti1516_2025::AttributeHandleSet{receiverPayRateAttribute}));
    if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(ownerRti->getObjectClassHandle(objectClassNameWide));
    } else {
      pumpEvoked(*ownerRti);
    }
    REQUIRE(ownerFederate.turnedOff);
    REQUIRE(ownerFederate.turnedOffObjectInstance == objectInstance);
    REQUIRE(ownerFederate.turnedOffAttributeCount == 1U);
    REQUIRE(ownerFederate.turnedOffAttributes.contains(ownerPayRateAttribute));

    ownerRti->resignFederationExecution(DELETE_OBJECTS);
    ownerJoined = false;
    receiverRti->resignFederationExecution(NO_ACTION);
    receiverJoined = false;
    ownerRti->disconnect();
    receiverRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (ownerJoined) {
      try {
        ownerRti->resignFederationExecution(DELETE_OBJECTS);
      } catch (...) {
      }
    }
    if (receiverJoined) {
      try {
        receiverRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      ownerRti->disconnect();
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
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(ownerJoined);
  REQUIRE_FALSE(receiverJoined);
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
