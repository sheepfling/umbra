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
    "RTIambassador routes regional process class Request Attribute Value Update with its region selector",
    "[integration][foundation][object-management][ddm][callbacks][callback-controls][transport][process-boundary][public-endpoint][rti.service.request-attribute-value-update-with-regions][rti.service.register-object-instance-with-regions][rti.service.create-region][rti.service.subscribe-object-class-attributes-with-regions][rti.service.update-attribute-values][federate.callback.provide-attribute-value-update][federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]") {
  auto runScenario = [&](CallbackModel callbackModel) {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void provideAttributeValueUpdate(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleSet const& attributes,
        rti1516_2025::VariableLengthData const& userSuppliedTag) override {
      providedObject = objectInstance;
      providedAttributeCount = attributes.size();
      if (userSuppliedTag.size() != 0U) {
        auto const* first = static_cast<std::uint8_t const*>(
            userSuppliedTag.data());
        providedTag.assign(first, first + userSuppliedTag.size());
      }
      ++providedCount;
    }

    rti1516_2025::ObjectInstanceHandle providedObject;
    std::size_t providedAttributeCount = 0U;
    std::vector<std::uint8_t> providedTag;
    std::size_t providedCount = 0U;
  } providerFederate;
  class RecordingRequesterFederateAmbassador final
      : public NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const& objectClass,
        std::wstring const&,
        rti1516_2025::FederateHandle const& producingFederate) override {
      discovered = true;
      discoveredObject = objectInstance;
      discoveredClass = objectClass;
      discoveredProducingFederate = producingFederate;
    }

    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
      reflected = true;
      reflectedObject = objectInstance;
      reflectedAttributeCount = attributeValues.size();
      reflectedTransportationType = transportationType;
      reflectedProducingFederate = producingFederate;
      reflectedHasSentRegions = optionalSentRegions != nullptr;
      reflectedSentRegionCount = optionalSentRegions == nullptr
          ? 0U
          : optionalSentRegions->size();
      reflectedTag.clear();
      if (userSuppliedTag.size() != 0U) {
        auto const* first = static_cast<std::uint8_t const*>(
            userSuppliedTag.data());
        reflectedTag.assign(first, first + userSuppliedTag.size());
      }
      reflectedValue.clear();
      if (!attributeValues.empty()) {
        auto const& value = attributeValues.begin()->second;
        if (value.size() != 0U) {
          auto const* first = static_cast<std::uint8_t const*>(value.data());
          reflectedValue.assign(first, first + value.size());
        }
      }
    }

    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObject;
    rti1516_2025::ObjectClassHandle discoveredClass;
    rti1516_2025::FederateHandle discoveredProducingFederate;
    bool reflected = false;
    rti1516_2025::ObjectInstanceHandle reflectedObject;
    std::size_t reflectedAttributeCount = 0U;
    rti1516_2025::TransportationTypeHandle reflectedTransportationType;
    rti1516_2025::FederateHandle reflectedProducingFederate;
    bool reflectedHasSentRegions = false;
    std::size_t reflectedSentRegionCount = 0U;
    std::vector<std::uint8_t> reflectedTag;
    std::vector<std::uint8_t> reflectedValue;
  } requesterFederate;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationNameWide =
      L"public-process-regional-class-attribute-execution";
  constexpr wchar_t const* objectClassNameWide =
      L"HLAobjectRoot.Food.Drink.Soda";
  constexpr char const* objectClassName = "HLAobjectRoot.Food.Drink.Soda";
  constexpr wchar_t const* attributeNameWide = L"Flavor";
  constexpr char const* attributeName = "Flavor";
  constexpr wchar_t const* dimensionNameWide = L"SodaFlavor";
  constexpr char const* dimensionName = "SodaFlavor";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::atomic_uint64_t expectedDimension{0U};
  std::atomic_uint64_t expectedProviderFederate{0U};
  std::atomic_uint64_t expectedRequesterFederate{0U};
  std::atomic_uint64_t expectedObjectInstance{0U};
  std::atomic_uint64_t recipientCount{
      std::numeric_limits<std::uint32_t>::max()};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          composedProcessDefinition(),
          ProcessFederationServiceOptions{callbackModel == HLA_IMMEDIATE});
      auto providerConnection = listener->accept(
          nullptr,
          {"public-process-regional-class-server", 0x9A21U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession provider(providerConnection);
      auto providerBaseHandler = service.handlerFor(provider);
      auto providerHandler = [&](TransportServiceMessage const& request) {
        auto response = providerBaseHandler(request);
        if (request.operation ==
                TransportServiceOperation::join_federation_execution &&
            response.status == TransportServiceStatus::ok) {
          auto const join = umbra::detail::decodeProcessFederationJoinResult(
              response.payload);
          expectedProviderFederate.store(
              join.federateId, std::memory_order_release);
        }
        if (request.operation ==
                TransportServiceOperation::register_object_instance_with_regions &&
            response.status == TransportServiceStatus::ok) {
          auto const registration =
              umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                  response.payload);
          expectedObjectInstance.store(
              registration.objectInstanceHandle, std::memory_order_release);
        }
        if (request.operation == TransportServiceOperation::update_attribute_values &&
            response.status == TransportServiceStatus::ok) {
          recipientCount.store(
              umbra::detail::decodeProcessFederationUpdateAttributeValuesResult(
                  response.payload)
                  .recipientCount,
              std::memory_order_release);
        }
        return response;
      };
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
          provider,
          providerHandler,
          TransportServiceOperation::create_federation_execution,
          "The regional process class server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationNameWide, objectClassName);
      auto const attribute = registry.attributeHandleFor(
          federationNameWide, objectClassName, attributeName);
      auto const dimension = registry.dimensionHandleFor(
          federationNameWide, dimensionName);
      if (!objectClass || !attribute || !dimension) {
        throw std::runtime_error(
            "The regional process class server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      expectedDimension.store(*dimension, std::memory_order_release);
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::join_federation_execution,
          "The regional process class server lost provider Join.");

      auto requesterConnection = listener->accept(
          nullptr,
          {"public-process-regional-class-server", 0x9A22U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession requester(requesterConnection);
      auto requesterBaseHandler = service.handlerFor(requester);
      auto requesterHandler = [&](TransportServiceMessage const& request) {
        auto response = requesterBaseHandler(request);
        if (request.operation ==
                TransportServiceOperation::join_federation_execution &&
            response.status == TransportServiceStatus::ok) {
          auto const join = umbra::detail::decodeProcessFederationJoinResult(
              response.payload);
          expectedRequesterFederate.store(
              join.federateId, std::memory_order_release);
        }
        if (request.operation ==
                TransportServiceOperation::request_attribute_value_update_class_with_regions &&
            response.status == TransportServiceStatus::ok) {
          recipientCount.store(
              umbra::detail::decodeProcessFederationRequestAttributeValueUpdateResult(
                  response.payload)
                  .recipientCount,
              std::memory_order_release);
        }
        return response;
      };
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::join_federation_execution,
          "The regional process class server lost requester Join.");

      for (auto const operation : {
               TransportServiceOperation::get_object_class_handle,
               TransportServiceOperation::get_attribute_handle,
               TransportServiceOperation::get_dimension_handle,
               TransportServiceOperation::get_dimension_upper_bound,
               TransportServiceOperation::create_region,
               TransportServiceOperation::get_dimension_handle_set,
               TransportServiceOperation::set_range_bounds,
               TransportServiceOperation::get_range_bounds,
               TransportServiceOperation::commit_region_modifications,
           }) {
        serveExpected(
            provider,
            providerHandler,
            operation,
            "The regional process class server lost provider DDM setup.");
      }
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The regional process class server lost Publish.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::register_object_instance_with_regions,
          "The regional process class server lost regional Register.");
      for (auto const operation : {
               TransportServiceOperation::get_object_class_handle,
               TransportServiceOperation::get_attribute_handle,
               TransportServiceOperation::get_dimension_handle,
               TransportServiceOperation::get_dimension_upper_bound,
               TransportServiceOperation::create_region,
               TransportServiceOperation::get_dimension_handle_set,
               TransportServiceOperation::set_range_bounds,
               TransportServiceOperation::get_range_bounds,
               TransportServiceOperation::commit_region_modifications,
           }) {
        serveExpected(
            requester,
            requesterHandler,
            operation,
            "The regional process class server lost requester DDM setup.");
      }
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::request_attribute_value_update_class_with_regions,
          "The regional process class server lost regional Request Attribute Value Update.");
      serveExpected(
          provider,
          providerHandler,
          callbackModel == HLA_IMMEDIATE
              ? TransportServiceOperation::get_object_class_handle
              : TransportServiceOperation::receive_interaction,
          "The regional process class server lost provider Provide polling.");
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::subscribe_object_class_attributes_with_regions,
          "The regional process class server lost requester regional Subscribe.");
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::update_attribute_values,
          "The regional process class server lost provider response Update.");
      if (callbackModel == HLA_IMMEDIATE) {
        serveExpected(
            requester,
            requesterHandler,
            TransportServiceOperation::get_object_class_handle,
            "The regional process class server lost requester immediate reflection polling.");
      } else {
        serveExpected(
            requester,
            requesterHandler,
            TransportServiceOperation::receive_interaction,
            "The regional process class server lost requester discovery polling.");
        serveExpected(
            requester,
            requesterHandler,
            TransportServiceOperation::receive_interaction,
            "The regional process class server lost requester reflection polling.");
      }
      serveExpected(
          provider,
          providerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The regional process class server lost provider Resign.");
      serveExpected(
          requester,
          requesterHandler,
          TransportServiceOperation::resign_federation_execution,
          "The regional process class server lost requester Resign.");
      service.detach(provider);
      service.detach(requester);
      providerConnection->close();
      requesterConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto providerRti = makeRti();
  auto requesterRti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"public-process-regional-class-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool providerJoined = false;
  bool requesterJoined = false;
  try {
    REQUIRE(providerRti->connect(providerFederate, callbackModel, configuration)
                .addressUsed);
    providerRti->createFederationExecution(
        federationNameWide, L"server-owned-fom.xml");
    providerRti->joinFederationExecution(
        L"public-process-regional-class-provider",
        L"public-process-regional-class-type",
        federationNameWide);
    providerJoined = true;

    REQUIRE(requesterRti->connect(requesterFederate, callbackModel, configuration)
                .addressUsed);
    requesterRti->joinFederationExecution(
        L"public-process-regional-class-requester",
        L"public-process-regional-class-type",
        federationNameWide);
    requesterJoined = true;

    auto const providerObjectClass =
        providerRti->getObjectClassHandle(objectClassNameWide);
    auto const providerAttribute =
        providerRti->getAttributeHandle(providerObjectClass, attributeNameWide);
    auto const providerDimension =
        providerRti->getDimensionHandle(dimensionNameWide);
    REQUIRE(providerObjectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(expectedObjectClass.load(
                    std::memory_order_acquire)) +
                L")");
    REQUIRE(providerAttribute ==
            rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                expectedAttribute.load(std::memory_order_acquire)));
    REQUIRE(providerDimension.toString() ==
            L"DimensionHandle(" +
                std::to_wstring(expectedDimension.load(
                    std::memory_order_acquire)) +
                L")");
    REQUIRE(providerRti->getDimensionUpperBound(providerDimension) == 4UL);

    auto const providerRegion =
        providerRti->createRegion(rti1516_2025::DimensionHandleSet{providerDimension});
    REQUIRE(providerRti->getDimensionHandleSet(providerRegion).contains(
        providerDimension));
    REQUIRE_NOTHROW(providerRti->setRangeBounds(
        providerRegion, providerDimension, rti1516_2025::RangeBounds(0UL, 2UL)));
    auto const providerBounds =
        providerRti->getRangeBounds(providerRegion, providerDimension);
    REQUIRE(providerBounds.getLowerBound() == 0UL);
    REQUIRE(providerBounds.getUpperBound() == 2UL);
    REQUIRE_NOTHROW(providerRti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{providerRegion}));

    providerRti->publishObjectClassAttributes(
        providerObjectClass,
        rti1516_2025::AttributeHandleSet{providerAttribute});
    rti1516_2025::AttributeHandleSetRegionHandleSetPairVector updatePairs;
    updatePairs.emplace_back(
        rti1516_2025::AttributeHandleSet{providerAttribute},
        rti1516_2025::RegionHandleSet{providerRegion});
    auto const objectInstance =
        providerRti->registerObjectInstanceWithRegions(
            providerObjectClass, updatePairs);
    REQUIRE(objectInstance.isValid());
    REQUIRE(objectInstance.toString() ==
            L"ObjectInstanceHandle(" +
                std::to_wstring(expectedObjectInstance.load(
                    std::memory_order_acquire)) +
                L")");

    auto const requesterObjectClass =
        requesterRti->getObjectClassHandle(objectClassNameWide);
    auto const requesterAttribute =
        requesterRti->getAttributeHandle(requesterObjectClass, attributeNameWide);
    auto const requesterDimension =
        requesterRti->getDimensionHandle(dimensionNameWide);
    REQUIRE(requesterObjectClass == providerObjectClass);
    REQUIRE(requesterAttribute == providerAttribute);
    REQUIRE(requesterDimension == providerDimension);
    REQUIRE(requesterRti->getDimensionUpperBound(requesterDimension) == 4UL);
    auto const requesterRegion =
        requesterRti->createRegion(rti1516_2025::DimensionHandleSet{requesterDimension});
    REQUIRE(requesterRti->getDimensionHandleSet(requesterRegion).contains(
        requesterDimension));
    REQUIRE_NOTHROW(requesterRti->setRangeBounds(
        requesterRegion,
        requesterDimension,
        rti1516_2025::RangeBounds(0UL, 2UL)));
    auto const requesterBounds =
        requesterRti->getRangeBounds(requesterRegion, requesterDimension);
    REQUIRE(requesterBounds.getLowerBound() == 0UL);
    REQUIRE(requesterBounds.getUpperBound() == 2UL);
    REQUIRE_NOTHROW(requesterRti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{requesterRegion}));

    rti1516_2025::AttributeHandleSetRegionHandleSetPairVector requestPairs;
    requestPairs.emplace_back(
        rti1516_2025::AttributeHandleSet{requesterAttribute},
        rti1516_2025::RegionHandleSet{requesterRegion});
    std::array<std::uint8_t, 3U> encodedTag{0x52U, 0x47U, 0x4EU};
    VariableLengthData userSuppliedTag(encodedTag.data(), encodedTag.size());
    REQUIRE_NOTHROW(requesterRti->requestAttributeValueUpdateWithRegions(
        requesterObjectClass, requestPairs, userSuppliedTag));
    if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(providerRti->getObjectClassHandle(objectClassNameWide));
    } else {
      static_cast<void>(providerRti->evokeCallback(0.0));
    }
    REQUIRE(providerFederate.providedCount == 1U);
    REQUIRE(providerFederate.providedObject == objectInstance);
    REQUIRE(providerFederate.providedAttributeCount == 1U);
    REQUIRE(providerFederate.providedTag ==
            std::vector<std::uint8_t>{0x52U, 0x47U, 0x4EU});
    REQUIRE_NOTHROW(requesterRti->subscribeObjectClassAttributesWithRegions(
        requesterObjectClass, requestPairs));

    std::array<std::uint8_t, 3U> encodedValue{0x52U, 0x53U, 0x50U};
    std::array<std::uint8_t, 3U> responseTag{0x52U, 0x53U, 0x54U};
    rti1516_2025::AttributeHandleValueMap responseValues;
    responseValues.emplace(
        providerAttribute,
        rti1516_2025::VariableLengthData(
            encodedValue.data(), encodedValue.size()));
    rti1516_2025::VariableLengthData responseTagData(
        responseTag.data(), responseTag.size());
    REQUIRE_NOTHROW(providerRti->updateAttributeValues(
        objectInstance, responseValues, responseTagData));
    if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(requesterRti->getObjectClassHandle(objectClassNameWide));
    } else {
      for (std::size_t evokeCount = 0U;
           evokeCount < 4U && !requesterFederate.reflected;
           ++evokeCount) {
        static_cast<void>(requesterRti->evokeCallback(0.0));
      }
    }
    REQUIRE(requesterFederate.discovered);
    REQUIRE(requesterFederate.discoveredObject == objectInstance);
    REQUIRE(requesterFederate.discoveredClass == requesterObjectClass);
    REQUIRE(requesterFederate.discoveredProducingFederate ==
            rti1516_2025::umbra_binding_detail::makeFederateHandle(
                expectedProviderFederate.load(std::memory_order_acquire)));
    REQUIRE(requesterFederate.reflected);
    REQUIRE(requesterFederate.reflectedObject == objectInstance);
    REQUIRE(requesterFederate.reflectedAttributeCount == 1U);
    REQUIRE(requesterFederate.reflectedValue ==
            std::vector<std::uint8_t>{0x52U, 0x53U, 0x50U});
    REQUIRE(requesterFederate.reflectedTag ==
            std::vector<std::uint8_t>{0x52U, 0x53U, 0x54U});
    REQUIRE(requesterFederate.reflectedTransportationType.isValid());
    REQUIRE(requesterFederate.reflectedProducingFederate ==
            rti1516_2025::umbra_binding_detail::makeFederateHandle(
                expectedProviderFederate.load(std::memory_order_acquire)));
    REQUIRE(requesterFederate.reflectedHasSentRegions);
    REQUIRE(requesterFederate.reflectedSentRegionCount == 1U);

    providerRti->resignFederationExecution(DELETE_OBJECTS);
    providerJoined = false;
    requesterRti->resignFederationExecution(NO_ACTION);
    requesterJoined = false;
    providerRti->disconnect();
    requesterRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (providerJoined) {
      try {
        providerRti->resignFederationExecution(DELETE_OBJECTS);
      } catch (...) {
      }
    }
    if (requesterJoined) {
      try {
        requesterRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      providerRti->disconnect();
    } catch (...) {
    }
    try {
      requesterRti->disconnect();
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
  REQUIRE(recipientCount.load(std::memory_order_acquire) == 1U);
  REQUIRE(expectedProviderFederate.load(std::memory_order_acquire) != 0U);
  REQUIRE(expectedRequesterFederate.load(std::memory_order_acquire) != 0U);
  REQUIRE_FALSE(providerJoined);
  REQUIRE_FALSE(requesterJoined);
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
