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
    "RTIambassador preserves a timestamped regional update through a configured process endpoint",
    "[integration][foundation][data-distribution-management][object-management][time-management][callbacks][transport][process-boundary][public-endpoint][rti.service.subscribe-object-class-attributes-with-regions][rti.service.register-object-instance-with-regions][rti.service.update-attribute-values][federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]") {
  auto runScenario = [](CallbackModel callbackModel) {
  class RecordingRegionalFederateAmbassador final : public NullFederateAmbassador {
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

    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
      ordinaryReceived = true;
      recordAttributeValues(
          objectInstance,
          attributeValues,
          userSuppliedTag,
          transportationType,
          producingFederate,
          optionalSentRegions);
    }

    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions,
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::OrderType sentOrder,
        rti1516_2025::OrderType receivedOrder,
        rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
      timestampedReceived = true;
      sentOrderType = sentOrder;
      receivedOrderType = receivedOrder;
      hasOptionalRetraction = optionalRetraction != nullptr;
      timestampImplementationName = time.implementationName();
      auto const encoded = time.encode();
      timestampEncoding.clear();
      if (encoded.size() != 0U) {
        auto const* first = static_cast<std::uint8_t const*>(encoded.data());
        timestampEncoding.assign(first, first + encoded.size());
      }
      recordAttributeValues(
          objectInstance,
          attributeValues,
          userSuppliedTag,
          transportationType,
          producingFederate,
          optionalSentRegions);
    }

    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    rti1516_2025::ObjectClassHandle discoveredObjectClass;
    std::wstring discoveredObjectInstanceName;
    rti1516_2025::FederateHandle discoveredProducingFederate;
    bool ordinaryReceived = false;
    bool timestampedReceived = false;
    rti1516_2025::ObjectInstanceHandle objectInstance;
    rti1516_2025::TransportationTypeHandle transportationType;
    rti1516_2025::FederateHandle producingFederate;
    std::size_t attributeCount = 0U;
    bool hasOptionalSentRegions = false;
    std::size_t sentRegionCount = 0U;
    std::vector<std::uint8_t> value;
    std::vector<std::uint8_t> tag;
    std::wstring timestampImplementationName;
    std::vector<std::uint8_t> timestampEncoding;
    rti1516_2025::OrderType sentOrderType = rti1516_2025::RECEIVE;
    rti1516_2025::OrderType receivedOrderType = rti1516_2025::RECEIVE;
    bool hasOptionalRetraction = false;

   private:
    void recordAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions) {
      this->objectInstance = objectInstance;
      this->transportationType = transportationType;
      this->producingFederate = producingFederate;
      this->attributeCount = attributeValues.size();
      this->hasOptionalSentRegions = optionalSentRegions != nullptr;
      this->sentRegionCount = optionalSentRegions == nullptr
          ? 0U
          : optionalSentRegions->size();
      this->tag.clear();
      if (userSuppliedTag.size() != 0U) {
        auto const* first = static_cast<std::uint8_t const*>(userSuppliedTag.data());
        this->tag.assign(first, first + userSuppliedTag.size());
      }
      this->value.clear();
      if (!attributeValues.empty()) {
        auto const& value = attributeValues.begin()->second;
        if (value.size() != 0U) {
          auto const* first = static_cast<std::uint8_t const*>(value.data());
          this->value.assign(first, first + value.size());
        }
      }
    }
  } receiverFederate;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-timestamped-regional-execution";
  constexpr char const* objectClassName = "HLAobjectRoot.Food.Drink.Soda";
  constexpr wchar_t const* objectClassNameWide =
      L"HLAobjectRoot.Food.Drink.Soda";
  constexpr char const* attributeName = "Flavor";
  constexpr wchar_t const* attributeNameWide = L"Flavor";
  constexpr char const* dimensionName = "SodaFlavor";
  constexpr wchar_t const* dimensionNameWide = L"SodaFlavor";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::atomic_uint64_t expectedDimension{0U};
  std::atomic_uint64_t expectedObjectInstance{0U};
  std::atomic_uint64_t expectedSenderFederate{0U};
  std::atomic_uint64_t recipientCount{std::numeric_limits<std::uint32_t>::max()};
  std::atomic_bool timestampPresent{false};
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
          {"public-process-timestamped-regional-server", 0x9D01U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderHandler = service.handlerFor(sender);
      auto receiverConnection = std::shared_ptr<ProcessTransportConnection>{};
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

      auto senderRecordingHandler = [&](TransportServiceMessage const& request) {
        if (request.operation == TransportServiceOperation::update_attribute_values) {
          auto const update =
              umbra::detail::decodeProcessFederationUpdateAttributeValuesRequest(
                  request.payload);
          timestampPresent.store(update.timestamp.has_value(), std::memory_order_release);
        }
        auto response = senderHandler(request);
        if (request.operation == TransportServiceOperation::join_federation_execution &&
            response.status == TransportServiceStatus::ok) {
          auto const join = umbra::detail::decodeProcessFederationJoinResult(
              response.payload);
          expectedSenderFederate.store(join.federateId, std::memory_order_release);
        }
        if (request.operation == TransportServiceOperation::register_object_instance_with_regions &&
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

      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::create_federation_execution,
          "The public timestamped regional server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationName, objectClassName);
      auto const attribute = registry.attributeHandleFor(
          federationName, objectClassName, attributeName);
      auto const dimension = registry.dimensionHandleFor(
          federationName, dimensionName);
      if (!objectClass || !attribute || !dimension) {
        throw std::runtime_error(
            "The public timestamped regional server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      expectedDimension.store(*dimension, std::memory_order_release);
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::join_federation_execution,
          "The public timestamped regional server lost sender Join.");

      receiverConnection = listener->accept(
          nullptr,
          {"public-process-timestamped-regional-server", 0x9D02U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The public timestamped regional server lost receiver Join.");

      for (auto const operation : {
               TransportServiceOperation::get_object_class_handle,
               TransportServiceOperation::get_attribute_handle,
               TransportServiceOperation::get_dimension_handle,
               // createRegion does not issue get_dimension_upper_bound.
               TransportServiceOperation::create_region,
               TransportServiceOperation::set_range_bounds,
               TransportServiceOperation::commit_region_modifications,
           }) {
        serveExpected(
            sender,
            senderRecordingHandler,
            operation,
            "The public timestamped regional server lost sender DDM setup.");
      }
      for (auto const operation : {
               TransportServiceOperation::get_object_class_handle,
               TransportServiceOperation::get_attribute_handle,
               TransportServiceOperation::get_dimension_handle,
               // createRegion does not issue get_dimension_upper_bound.
               TransportServiceOperation::create_region,
               TransportServiceOperation::set_range_bounds,
               TransportServiceOperation::commit_region_modifications,
               TransportServiceOperation::subscribe_object_class_attributes_with_regions,
           }) {
        serveExpected(
            receiver,
            receiverHandler,
            operation,
            "The public timestamped regional server lost receiver DDM setup.");
      }
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The public timestamped regional server lost Publish.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::register_object_instance_with_regions,
          "The public timestamped regional server lost regional Register.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::update_attribute_values,
          "The public timestamped regional server lost regional Update.");
      if (callbackModel == HLA_IMMEDIATE) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_object_class_handle,
            "The public timestamped regional server lost immediate event polling.");
      } else {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::receive_interaction,
            "The public timestamped regional server lost discovery Receive.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::receive_interaction,
            "The public timestamped regional server lost reflection Receive.");
      }
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public timestamped regional server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public timestamped regional server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador senderFederate;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"public-process-timestamped-regional-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(senderFederate, callbackModel, configuration).addressUsed);
    senderRti->createFederationExecution(federationName, L"server-owned-fom.xml");
    auto const senderHandle = senderRti->joinFederationExecution(
        L"public-process-timestamped-regional-sender",
        L"public-process-timestamped-regional-type",
        federationName);
    senderJoined = true;

    REQUIRE(receiverRti->connect(receiverFederate, callbackModel, configuration).addressUsed);
    receiverRti->joinFederationExecution(
        L"public-process-timestamped-regional-receiver",
        L"public-process-timestamped-regional-type",
        federationName);
    receiverJoined = true;

    auto const senderObjectClass = senderRti->getObjectClassHandle(objectClassNameWide);
    auto const senderAttribute = senderRti->getAttributeHandle(
        senderObjectClass, attributeNameWide);
    auto const senderDimension = senderRti->getDimensionHandle(dimensionNameWide);
    REQUIRE(senderObjectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    REQUIRE(senderAttribute == rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                                  expectedAttribute.load(std::memory_order_acquire)));
    REQUIRE(senderDimension.toString() ==
            L"DimensionHandle(" +
                std::to_wstring(expectedDimension.load(std::memory_order_acquire)) +
                L")");
    auto const senderRegion = senderRti->createRegion(
        rti1516_2025::DimensionHandleSet{senderDimension});
    REQUIRE(senderRegion.isValid());
    REQUIRE_NOTHROW(senderRti->setRangeBounds(
        senderRegion, senderDimension, rti1516_2025::RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(senderRti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{senderRegion}));

    auto const receiverObjectClass = receiverRti->getObjectClassHandle(objectClassNameWide);
    auto const receiverAttribute = receiverRti->getAttributeHandle(
        receiverObjectClass, attributeNameWide);
    auto const receiverDimension = receiverRti->getDimensionHandle(dimensionNameWide);
    auto const receiverRegion = receiverRti->createRegion(
        rti1516_2025::DimensionHandleSet{receiverDimension});
    REQUIRE(receiverRegion.isValid());
    REQUIRE_NOTHROW(receiverRti->setRangeBounds(
        receiverRegion, receiverDimension, rti1516_2025::RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(receiverRti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{receiverRegion}));
    rti1516_2025::AttributeHandleSet const receiverAttributes{receiverAttribute};
    rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const receiverPairs{{
        receiverAttributes,
        rti1516_2025::RegionHandleSet{receiverRegion},
    }};
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributesWithRegions(
        receiverObjectClass,
        receiverPairs));

    rti1516_2025::AttributeHandleSet const senderAttributes{senderAttribute};
    REQUIRE_NOTHROW(senderRti->publishObjectClassAttributes(
        senderObjectClass, senderAttributes));
    auto const objectInstance = senderRti->registerObjectInstanceWithRegions(
        senderObjectClass,
        rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
            senderAttributes,
            rti1516_2025::RegionHandleSet{senderRegion},
        }});
    REQUIRE(objectInstance.isValid());

    auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
        L"HLAinteger64Time");
    auto* integerFactory =
        dynamic_cast<rti1516_2025::HLAinteger64TimeFactory*>(factory.get());
    REQUIRE(integerFactory != nullptr);
    auto timestamp = integerFactory->makeLogicalTime(5);
    REQUIRE(timestamp);
    auto const expectedTimestampEncoding = timestamp->encode();

    std::array<std::uint8_t, 3U> encodedValue{0x54U, 0x53U, 0x4FU};
    std::array<std::uint8_t, 3U> encodedTag{0x54U, 0x52U, 0x47U};
    rti1516_2025::AttributeHandleValueMap attributeValues;
    attributeValues.emplace(
        senderAttribute,
        rti1516_2025::VariableLengthData(encodedValue.data(), encodedValue.size()));
    rti1516_2025::VariableLengthData userSuppliedTag(
        encodedTag.data(), encodedTag.size());
    auto const retraction = senderRti->updateAttributeValues(
        objectInstance, attributeValues, userSuppliedTag, *timestamp);
    REQUIRE_FALSE(retraction.isValid());

    if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(receiverRti->getObjectClassHandle(objectClassNameWide));
    } else {
      for (std::size_t evokeCount = 0U; evokeCount < 5U &&
           !receiverFederate.timestampedReceived; ++evokeCount) {
        static_cast<void>(receiverRti->evokeCallback(0.0));
      }
    }
    REQUIRE(receiverFederate.discovered);
    REQUIRE(receiverFederate.timestampedReceived);
    REQUIRE_FALSE(receiverFederate.ordinaryReceived);
    REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);
    REQUIRE(receiverFederate.objectInstance == objectInstance);
    REQUIRE(receiverFederate.attributeCount == 1U);
    REQUIRE(receiverFederate.value == std::vector<std::uint8_t>{0x54U, 0x53U, 0x4FU});
    REQUIRE(receiverFederate.tag == std::vector<std::uint8_t>{0x54U, 0x52U, 0x47U});
    REQUIRE(receiverFederate.hasOptionalSentRegions);
    REQUIRE(receiverFederate.sentRegionCount == 1U);
    REQUIRE(receiverFederate.transportationType.isValid());
    REQUIRE(receiverFederate.producingFederate == senderHandle);
    REQUIRE(receiverFederate.timestampImplementationName == L"HLAinteger64Time");
    std::vector<std::uint8_t> expectedTimestampBytes;
    if (expectedTimestampEncoding.size() != 0U) {
      auto const* first = static_cast<std::uint8_t const*>(expectedTimestampEncoding.data());
      expectedTimestampBytes.assign(
          first, first + expectedTimestampEncoding.size());
    }
    REQUIRE(receiverFederate.timestampEncoding == expectedTimestampBytes);
    REQUIRE(receiverFederate.sentOrderType == rti1516_2025::RECEIVE);
    REQUIRE(receiverFederate.receivedOrderType == rti1516_2025::RECEIVE);
    REQUIRE_FALSE(receiverFederate.hasOptionalRetraction);
    REQUIRE(timestampPresent.load(std::memory_order_acquire));
    REQUIRE(recipientCount.load(std::memory_order_acquire) == 1U);
    REQUIRE(expectedObjectInstance.load(std::memory_order_acquire) != 0U);
    REQUIRE(expectedSenderFederate.load(std::memory_order_acquire) != 0U);

    senderRti->resignFederationExecution(DELETE_OBJECTS);
    senderJoined = false;
    receiverRti->resignFederationExecution(NO_ACTION);
    receiverJoined = false;
    senderRti->disconnect();
    receiverRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (senderJoined) {
      try {
        senderRti->resignFederationExecution(DELETE_OBJECTS);
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
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(senderJoined);
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
