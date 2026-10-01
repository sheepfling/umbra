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
    "RTIambassadors preserve regional scope while retracting a middle timestamped process attribute",
    "[integration][foundation][data-distribution-management][object-management][time-management][time-advance]"
    "[transport][process-boundary][public-endpoint][multi-federate][tso][retraction][regional]"
    "[timestamped-process-attribute-update][two-recipient-fanout][process-tso-attribute-retraction-regional]"
    "[rti.service.create-region][rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions][rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const&,
        std::wstring const&,
        rti1516_2025::FederateHandle const&) override {
      discovered = true;
      discoveredObjectInstance = objectInstance;
    }

    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const&,
        rti1516_2025::TransportationTypeHandle const&,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions,
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::OrderType sentOrder,
        rti1516_2025::OrderType receivedOrder,
        rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
      ++reflectedCount;
      reflectedObjectInstance = objectInstance;
      reflectedProducingFederate = producingFederate;
      reflectedAttributeCounts.push_back(attributeValues.size());
      reflectedSentOrders.push_back(sentOrder);
      reflectedReceivedOrders.push_back(receivedOrder);
      reflectedHasRetraction.push_back(
          optionalRetraction != nullptr && optionalRetraction->isValid());
      reflectedHasRegions.push_back(optionalSentRegions != nullptr);
      reflectedRegionCounts.push_back(
          optionalSentRegions == nullptr ? 0U : optionalSentRegions->size());
      auto const* integerTime =
          dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time);
      REQUIRE(integerTime != nullptr);
      reflectedTimestampValues.push_back(integerTime->getTime());
      callbackOrder.push_back('R');
    }

    void timeRegulationEnabled(rti1516_2025::LogicalTime const&) override {
      ++timeRegulationEnabledCount;
    }

    void timeConstrainedEnabled(rti1516_2025::LogicalTime const&) override {
      ++timeConstrainedEnabledCount;
    }

    void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
      ++timeAdvanceGrantCount;
      auto const* integerTime =
          dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time);
      REQUIRE(integerTime != nullptr);
      timeAdvanceGrantValues.push_back(integerTime->getTime());
      callbackOrder.push_back('G');
    }

    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    std::size_t reflectedCount = 0U;
    rti1516_2025::ObjectInstanceHandle reflectedObjectInstance;
    rti1516_2025::FederateHandle reflectedProducingFederate;
    std::vector<std::size_t> reflectedAttributeCounts;
    std::vector<rti1516_2025::OrderType> reflectedSentOrders;
    std::vector<rti1516_2025::OrderType> reflectedReceivedOrders;
    std::vector<bool> reflectedHasRetraction;
    std::vector<bool> reflectedHasRegions;
    std::vector<std::size_t> reflectedRegionCounts;
    std::vector<std::int64_t> reflectedTimestampValues;
    std::size_t timeRegulationEnabledCount = 0U;
    std::size_t timeConstrainedEnabledCount = 0U;
    std::size_t timeAdvanceGrantCount = 0U;
    std::vector<std::int64_t> timeAdvanceGrantValues;
    std::vector<char> callbackOrder;
  } senderFederate, receiverOneFederate, receiverTwoFederate;

  constexpr wchar_t const* federationName =
      L"process-timestamped-attribute-retraction-regional-execution";
  constexpr wchar_t const* senderName =
      L"process-timestamped-attribute-retraction-regional-sender";
  constexpr wchar_t const* receiverOneName =
      L"process-timestamped-attribute-retraction-regional-receiver-one";
  constexpr wchar_t const* receiverTwoName =
      L"process-timestamped-attribute-retraction-regional-receiver-two";
  constexpr wchar_t const* federateType =
      L"process-timestamped-attribute-retraction-regional-type";
  constexpr wchar_t const* objectClassName =
      L"HLAobjectRoot.Food.Drink.Soda";
  constexpr wchar_t const* attributeName = L"Flavor";
  constexpr wchar_t const* dimensionName = L"SodaFlavor";

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::atomic_uint64_t expectedDimension{0U};
  std::atomic_uint64_t expectedSenderFederate{0U};
  std::atomic_uint64_t expectedObjectInstance{0U};
  std::atomic_uint64_t recipientCount{
      std::numeric_limits<std::uint32_t>::max()};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"process-timestamped-attribute-retraction-regional-server", 0x9390U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto const senderBaseHandler = service.handlerFor(sender);
      auto senderHandler = [&](TransportServiceMessage const& request) {
        auto response = senderBaseHandler(request);
        if (request.operation == TransportServiceOperation::join_federation_execution &&
            response.status == TransportServiceStatus::ok) {
          auto const join =
              umbra::detail::decodeProcessFederationJoinResult(response.payload);
          expectedSenderFederate.store(join.federateId, std::memory_order_release);
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
          sender,
          senderHandler,
          TransportServiceOperation::create_federation_execution,
          "The regional process server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationName, "HLAobjectRoot.Food.Drink.Soda");
      auto const attribute = registry.attributeHandleFor(
          federationName, "HLAobjectRoot.Food.Drink.Soda", "Flavor");
      auto const dimension = registry.dimensionHandleFor(
          federationName, "SodaFlavor");
      if (!objectClass || !attribute || !dimension) {
        throw std::runtime_error(
            "The regional process server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      expectedDimension.store(*dimension, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The regional process server lost sender Join.");

      auto receiverOneConnection = listener->accept(
          nullptr,
          {"process-timestamped-attribute-retraction-regional-server", 0x9391U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiverOne(receiverOneConnection);
      auto const receiverOneBaseHandler = service.handlerFor(receiverOne);
      auto receiverOneHandler = [&](TransportServiceMessage const& request) {
        return receiverOneBaseHandler(request);
      };
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::join_federation_execution,
          "The regional process server lost receiver-one Join.");

      auto receiverTwoConnection = listener->accept(
          nullptr,
          {"process-timestamped-attribute-retraction-regional-server", 0x9392U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiverTwo(receiverTwoConnection);
      auto const receiverTwoBaseHandler = service.handlerFor(receiverTwo);
      auto receiverTwoHandler = [&](TransportServiceMessage const& request) {
        return receiverTwoBaseHandler(request);
      };
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::join_federation_execution,
          "The regional process server lost receiver-two Join.");

      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_object_class_handle,
          "The regional process server lost sender object-class lookup.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::get_object_class_handle,
          "The regional process server lost receiver-one object-class lookup.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::get_object_class_handle,
          "The regional process server lost receiver-two object-class lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_attribute_handle,
          "The regional process server lost sender attribute lookup.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::get_attribute_handle,
          "The regional process server lost receiver-one attribute lookup.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::get_attribute_handle,
          "The regional process server lost receiver-two attribute lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_dimension_handle,
          "The regional process server lost sender dimension lookup.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::get_dimension_handle,
          "The regional process server lost receiver-one dimension lookup.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::get_dimension_handle,
          "The regional process server lost receiver-two dimension lookup.");

      for (auto const operation : {
               TransportServiceOperation::create_region,
               TransportServiceOperation::set_range_bounds,
               TransportServiceOperation::commit_region_modifications,
           }) {
        serveExpected(
            sender,
            senderHandler,
            operation,
            "The regional process server lost sender region setup.");
      }
      for (auto const operation : {
               TransportServiceOperation::create_region,
               TransportServiceOperation::set_range_bounds,
               TransportServiceOperation::commit_region_modifications,
           }) {
        serveExpected(
            receiverOne,
            receiverOneHandler,
            operation,
            "The regional process server lost receiver-one region setup.");
      }
      for (auto const operation : {
               TransportServiceOperation::create_region,
               TransportServiceOperation::set_range_bounds,
               TransportServiceOperation::commit_region_modifications,
           }) {
        serveExpected(
            receiverTwo,
            receiverTwoHandler,
            operation,
            "The regional process server lost receiver-two region setup.");
      }
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::set_convey_region_designator_sets_switch,
          "The regional process server lost receiver-one Convey Region Designator Sets.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::set_convey_region_designator_sets_switch,
          "The regional process server lost receiver-two Convey Region Designator Sets.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::subscribe_object_class_attributes_with_regions,
          "The regional process server lost receiver-one regional Subscribe.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::subscribe_object_class_attributes_with_regions,
          "The regional process server lost receiver-two regional Subscribe.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::enable_time_regulation,
          "The regional process server lost Enable Time Regulation.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::enable_time_constrained,
          "The regional process server lost receiver-one Enable Time Constrained.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::enable_time_constrained,
          "The regional process server lost receiver-two Enable Time Constrained.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The regional process server lost Publish.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::register_object_instance_with_regions,
          "The regional process server lost regional Register.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::receive_interaction,
          "The regional process server lost receiver-one discovery polling.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::receive_interaction,
          "The regional process server lost receiver-two discovery polling.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::update_attribute_values,
          "The regional process server lost timestamp-5 Update.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::update_attribute_values,
          "The regional process server lost timestamp-6 Update.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::update_attribute_values,
          "The regional process server lost timestamp-7 Update.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::retract,
          "The regional process server lost middle Retract.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::time_advance_request,
          "The regional process server lost receiver-one TAR at timestamp 5.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::time_advance_request,
          "The regional process server lost receiver-two TAR at timestamp 5.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The regional process server lost sender intermediate TAR.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The regional process server lost sender timestamp-5 TAR.");

      auto serveCallbacks = [&](ProcessTransportSession& session,
                                auto const& handler,
                                std::size_t& polls,
                                std::size_t& acknowledgements,
                                std::size_t target,
                                char const* description) {
        while (acknowledgements < target && polls != 12U) {
          if (!umbra::test::servePrimaryProcessRequest(
                  session, handler,
                  [&](TransportServiceMessage const& request) {
                    if (request.operation ==
                        TransportServiceOperation::receive_interaction) {
                      ++polls;
                      return handler(request);
                    }
                    if (request.operation ==
                        TransportServiceOperation::acknowledge_tso_delivery) {
                      ++acknowledgements;
                      return handler(request);
                    }
                    throw std::runtime_error(
                        std::string(
                            "The regional process server received an unexpected callback operation ") +
                        std::to_string(static_cast<unsigned>(request.operation)) +
                        ".");
                  })) {
            throw std::runtime_error(
                "The regional process server lost a receiver callback operation.");
          }
        }
        if (acknowledgements != target) {
          throw std::runtime_error(description);
        }
      };
      std::size_t receiverOnePolls = 0U;
      std::size_t receiverOneAcknowledgements = 0U;
      std::size_t receiverTwoPolls = 0U;
      std::size_t receiverTwoAcknowledgements = 0U;
      serveCallbacks(
          receiverOne,
          receiverOneHandler,
          receiverOnePolls,
          receiverOneAcknowledgements,
          1U,
          "The regional process server did not receive receiver-one timestamp-5 acknowledgement.");
      serveCallbacks(
          receiverTwo,
          receiverTwoHandler,
          receiverTwoPolls,
          receiverTwoAcknowledgements,
          1U,
          "The regional process server did not receive receiver-two timestamp-5 acknowledgement.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::time_advance_request,
          "The regional process server lost receiver-one TAR at timestamp 7.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::time_advance_request,
          "The regional process server lost receiver-two TAR at timestamp 7.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The regional process server lost sender final TAR.");
      serveCallbacks(
          receiverOne,
          receiverOneHandler,
          receiverOnePolls,
          receiverOneAcknowledgements,
          2U,
          "The regional process server did not receive receiver-one timestamp-7 acknowledgement.");
      serveCallbacks(
          receiverTwo,
          receiverTwoHandler,
          receiverTwoPolls,
          receiverTwoAcknowledgements,
          2U,
          "The regional process server did not receive receiver-two timestamp-7 acknowledgement.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The regional process server lost sender Resign.");
      serveExpected(
          receiverOne,
          receiverOneHandler,
          TransportServiceOperation::resign_federation_execution,
          "The regional process server lost receiver-one Resign.");
      serveExpected(
          receiverTwo,
          receiverTwoHandler,
          TransportServiceOperation::resign_federation_execution,
          "The regional process server lost receiver-two Resign.");
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

  auto senderRti = makeRti();
  auto receiverOneRti = makeRti();
  auto receiverTwoRti = makeRti();
  auto senderConfiguration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"process-timestamped-attribute-retraction-regional-sender-client")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto receiverOneConfiguration = RtiConfiguration::createConfiguration()
                                      .withConfigurationName(
                                          L"process-timestamped-attribute-retraction-regional-receiver-one-client")
                                      .withRtiAddress(
                                          L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto receiverTwoConfiguration = RtiConfiguration::createConfiguration()
                                      .withConfigurationName(
                                          L"process-timestamped-attribute-retraction-regional-receiver-two-client")
                                      .withRtiAddress(
                                          L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverOneJoined = false;
  bool receiverTwoJoined = false;
  try {
    REQUIRE(senderRti->connect(
                senderFederate, HLA_EVOKED, senderConfiguration)
                .addressUsed);
    REQUIRE_NOTHROW(senderRti->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    auto const senderHandle = senderRti->joinFederationExecution(
        senderName, federateType, federationName);
    REQUIRE(senderHandle.isValid());
    senderJoined = true;
    REQUIRE(receiverOneRti->connect(
                receiverOneFederate, HLA_EVOKED, receiverOneConfiguration)
                .addressUsed);
    auto const receiverOneHandle = receiverOneRti->joinFederationExecution(
        receiverOneName, federateType, federationName);
    REQUIRE(receiverOneHandle.isValid());
    receiverOneJoined = true;
    REQUIRE(receiverTwoRti->connect(
                receiverTwoFederate, HLA_EVOKED, receiverTwoConfiguration)
                .addressUsed);
    auto const receiverTwoHandle = receiverTwoRti->joinFederationExecution(
        receiverTwoName, federateType, federationName);
    REQUIRE(receiverTwoHandle.isValid());
    receiverTwoJoined = true;

    auto const senderObjectClass = senderRti->getObjectClassHandle(objectClassName);
    auto const receiverOneObjectClass =
        receiverOneRti->getObjectClassHandle(objectClassName);
    auto const receiverTwoObjectClass =
        receiverTwoRti->getObjectClassHandle(objectClassName);
    auto const senderAttribute =
        senderRti->getAttributeHandle(senderObjectClass, attributeName);
    auto const receiverOneAttribute =
        receiverOneRti->getAttributeHandle(receiverOneObjectClass, attributeName);
    auto const receiverTwoAttribute =
        receiverTwoRti->getAttributeHandle(receiverTwoObjectClass, attributeName);
    auto const senderDimension = senderRti->getDimensionHandle(dimensionName);
    auto const receiverOneDimension =
        receiverOneRti->getDimensionHandle(dimensionName);
    auto const receiverTwoDimension =
        receiverTwoRti->getDimensionHandle(dimensionName);
    REQUIRE(senderObjectClass == receiverOneObjectClass);
    REQUIRE(senderObjectClass == receiverTwoObjectClass);
    REQUIRE(senderObjectClass ==
            rti1516_2025::umbra_binding_detail::makeObjectClassHandle(
                expectedObjectClass.load(std::memory_order_acquire)));
    REQUIRE(senderAttribute == receiverOneAttribute);
    REQUIRE(senderAttribute == receiverTwoAttribute);
    REQUIRE(senderAttribute ==
            rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                expectedAttribute.load(std::memory_order_acquire)));
    REQUIRE(senderDimension.isValid());
    REQUIRE(receiverOneDimension.isValid());
    REQUIRE(receiverTwoDimension.isValid());
    REQUIRE(senderDimension.toString() ==
            L"DimensionHandle(" +
                std::to_wstring(expectedDimension.load(
                    std::memory_order_acquire)) +
                L")");

    auto makeRegion = [](RTIambassador& rti,
                         rti1516_2025::DimensionHandle const& dimension) {
      auto const region =
          rti.createRegion(rti1516_2025::DimensionHandleSet{dimension});
      REQUIRE(region.isValid());
      REQUIRE_NOTHROW(rti.setRangeBounds(
          region, dimension, rti1516_2025::RangeBounds(0UL, 1UL)));
      REQUIRE_NOTHROW(rti.commitRegionModifications(
          rti1516_2025::RegionHandleSet{region}));
      return region;
    };
    auto const senderRegion = makeRegion(*senderRti, senderDimension);
    auto const receiverOneRegion =
        makeRegion(*receiverOneRti, receiverOneDimension);
    auto const receiverTwoRegion =
        makeRegion(*receiverTwoRti, receiverTwoDimension);
    rti1516_2025::AttributeHandleSet const senderAttributes{senderAttribute};
    rti1516_2025::AttributeHandleSet const receiverOneAttributes{
        receiverOneAttribute};
    rti1516_2025::AttributeHandleSet const receiverTwoAttributes{
        receiverTwoAttribute};
    REQUIRE_NOTHROW(receiverOneRti->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE_NOTHROW(receiverTwoRti->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE_NOTHROW(receiverOneRti->subscribeObjectClassAttributesWithRegions(
        receiverOneObjectClass,
        rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
            receiverOneAttributes,
            rti1516_2025::RegionHandleSet{receiverOneRegion},
        }}));
    REQUIRE_NOTHROW(receiverTwoRti->subscribeObjectClassAttributesWithRegions(
        receiverTwoObjectClass,
        rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
            receiverTwoAttributes,
            rti1516_2025::RegionHandleSet{receiverTwoRegion},
        }}));
    REQUIRE_NOTHROW(senderRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeRegulationEnabledCount == 1U);
    REQUIRE_NOTHROW(receiverOneRti->enableTimeConstrained());
    REQUIRE_FALSE(receiverOneRti->evokeCallback(0.0));
    REQUIRE(receiverOneFederate.timeConstrainedEnabledCount == 1U);
    REQUIRE_NOTHROW(receiverTwoRti->enableTimeConstrained());
    REQUIRE_FALSE(receiverTwoRti->evokeCallback(0.0));
    REQUIRE(receiverTwoFederate.timeConstrainedEnabledCount == 1U);
    REQUIRE_NOTHROW(senderRti->publishObjectClassAttributes(
        senderObjectClass, senderAttributes));
    auto const objectInstance = senderRti->registerObjectInstanceWithRegions(
        senderObjectClass,
        rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
            senderAttributes,
            rti1516_2025::RegionHandleSet{senderRegion},
        }});
    REQUIRE(objectInstance.isValid());
    REQUIRE(objectInstance ==
            rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle(
                expectedObjectInstance.load(std::memory_order_acquire)));
    REQUIRE_FALSE(receiverOneFederate.discovered);
    REQUIRE_FALSE(receiverTwoFederate.discovered);
    REQUIRE_FALSE(receiverOneRti->evokeCallback(0.0));
    REQUIRE_FALSE(receiverTwoRti->evokeCallback(0.0));
    REQUIRE(receiverOneFederate.discovered);
    REQUIRE(receiverTwoFederate.discovered);
    REQUIRE(receiverOneFederate.discoveredObjectInstance == objectInstance);
    REQUIRE(receiverTwoFederate.discoveredObjectInstance == objectInstance);

    auto sendAt = [&](std::uint8_t value, std::uint8_t tag, std::int64_t time) {
      AttributeHandleValueMap values;
      values.emplace(
          senderAttribute,
          VariableLengthData(&value, sizeof(value)));
      VariableLengthData userSuppliedTag(&tag, sizeof(tag));
      auto const retraction = senderRti->updateAttributeValues(
          objectInstance,
          values,
          userSuppliedTag,
          rti1516_2025::HLAinteger64Time(time));
      REQUIRE(retraction.isValid());
      return retraction;
    };
    auto const first = sendAt(0x35U, 0xA5U, 5);
    auto const middle = sendAt(0x36U, 0xA6U, 6);
    auto const last = sendAt(0x37U, 0xA7U, 7);
    REQUIRE(first.isValid());
    REQUIRE(middle.isValid());
    REQUIRE(last.isValid());
    REQUIRE_NOTHROW(senderRti->retract(middle));
    REQUIRE(receiverOneFederate.reflectedCount == 0U);
    REQUIRE(receiverTwoFederate.reflectedCount == 0U);

    REQUIRE_NOTHROW(receiverOneRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE_NOTHROW(receiverTwoRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(2)));
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeAdvanceGrantValues == std::vector<std::int64_t>{2});
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeAdvanceGrantValues ==
            std::vector<std::int64_t>{2, 5});

    auto evokeUntil = [](RTIambassador& rti,
                         RecordingFederateAmbassador& ambassador,
                         std::size_t targetReflections,
                         std::size_t targetGrants) {
      for (int pass = 0;
           pass != 12 &&
           (ambassador.reflectedCount < targetReflections ||
            ambassador.timeAdvanceGrantCount < targetGrants);
           ++pass) {
        static_cast<void>(rti.evokeCallback(0.0));
      }
    };
    evokeUntil(*receiverOneRti, receiverOneFederate, 1U, 1U);
    evokeUntil(*receiverTwoRti, receiverTwoFederate, 1U, 1U);
    for (auto const* receiver : {&receiverOneFederate, &receiverTwoFederate}) {
      REQUIRE(receiver->reflectedTimestampValues ==
              std::vector<std::int64_t>{5});
      REQUIRE(receiver->timeAdvanceGrantValues ==
              std::vector<std::int64_t>{5});
      REQUIRE(receiver->callbackOrder == std::vector<char>{'R', 'G'});
    }

    REQUIRE_NOTHROW(receiverOneRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_NOTHROW(receiverTwoRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeAdvanceGrantValues ==
            std::vector<std::int64_t>{2, 5, 7});
    evokeUntil(*receiverOneRti, receiverOneFederate, 2U, 2U);
    evokeUntil(*receiverTwoRti, receiverTwoFederate, 2U, 2U);
    for (auto const* receiver : {&receiverOneFederate, &receiverTwoFederate}) {
      REQUIRE(receiver->reflectedTimestampValues ==
              std::vector<std::int64_t>{5, 7});
      REQUIRE(receiver->timeAdvanceGrantValues ==
              std::vector<std::int64_t>{5, 7});
      REQUIRE(receiver->callbackOrder ==
              std::vector<char>{'R', 'G', 'R', 'G'});
      REQUIRE(receiver->reflectedAttributeCounts ==
              std::vector<std::size_t>{1U, 1U});
      REQUIRE(receiver->reflectedSentOrders ==
              std::vector<rti1516_2025::OrderType>{
                  rti1516_2025::TIMESTAMP, rti1516_2025::TIMESTAMP});
      REQUIRE(receiver->reflectedReceivedOrders ==
              std::vector<rti1516_2025::OrderType>{
                  rti1516_2025::TIMESTAMP, rti1516_2025::TIMESTAMP});
      REQUIRE(receiver->reflectedHasRetraction ==
              std::vector<bool>{true, true});
      REQUIRE(receiver->reflectedHasRegions ==
              std::vector<bool>{true, true});
      REQUIRE(receiver->reflectedRegionCounts ==
              std::vector<std::size_t>{1U, 1U});
      REQUIRE(receiver->reflectedObjectInstance == objectInstance);
      REQUIRE(receiver->reflectedProducingFederate == senderHandle);
    }
    REQUIRE(recipientCount.load(std::memory_order_acquire) == 2U);
    REQUIRE(expectedSenderFederate.load(std::memory_order_acquire) != 0U);
    REQUIRE_NOTHROW(senderRti->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
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
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (serverError) {
    try {
      std::rethrow_exception(serverError);
    } catch (std::exception const& error) {
      FAIL_CHECK(std::string("regional process server: ") + error.what());
    } catch (...) {
      FAIL_CHECK("regional process server failed with an unknown exception");
    }
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
}

#endif
#endif
