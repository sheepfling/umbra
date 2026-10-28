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
#include "process_service_report_fixture.hpp"
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
enum class ProcessTimeRequest { tar, nmr, nmra };

void verifyProcessTsoAttributeBeforeGrant(
    ProcessTimeRequest requestKind,
    bool const regionalAttribute = false,
    CallbackModel const callbackModel = HLA_EVOKED,
    bool const pendingTceTransition = false) {
  REQUIRE_FALSE((pendingTceTransition &&
                (requestKind != ProcessTimeRequest::tar || regionalAttribute ||
                 callbackModel != HLA_EVOKED)));

  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const&,
        std::wstring const&,
        rti1516_2025::FederateHandle const&) override {
      std::scoped_lock lock(callbackMutex);
      discoveredObjectInstance = objectInstance;
      discovered = true;
      callbackChanged.notify_all();
    }

    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const&,
        rti1516_2025::FederateHandle const&,
        rti1516_2025::RegionHandleSet const*,
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::OrderType sentOrder,
        rti1516_2025::OrderType receivedOrder,
        rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
      std::scoped_lock lock(callbackMutex);
      ++reflectedCount;
      callbackOrder.push_back('A');
      reflectedObjectInstance = objectInstance;
      reflectedAttributeCount = attributeValues.size();
      sentOrderType = sentOrder;
      receivedOrderType = receivedOrder;
      hasRetraction = optionalRetraction != nullptr && optionalRetraction->isValid();
      reflectedTimestampValue = -1;
      if (auto const* integerTime =
              dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
        reflectedTimestampValue = integerTime->getTime();
      }
      reflectedTag.clear();
      if (userSuppliedTag.size() != 0U) {
        auto const* data = static_cast<std::uint8_t const*>(userSuppliedTag.data());
        reflectedTag.assign(data, data + userSuppliedTag.size());
      }
      reflectedValue.clear();
      if (!attributeValues.empty() && attributeValues.begin()->second.size() != 0U) {
        auto const& value = attributeValues.begin()->second;
        auto const* data = static_cast<std::uint8_t const*>(value.data());
        reflectedValue.assign(data, data + value.size());
      }
      callbackChanged.notify_all();
    }

    void timeRegulationEnabled(rti1516_2025::LogicalTime const&) override {
      std::scoped_lock lock(callbackMutex);
      ++timeRegulationEnabledCount;
      callbackChanged.notify_all();
    }

    void timeConstrainedEnabled(rti1516_2025::LogicalTime const&) override {
      std::scoped_lock lock(callbackMutex);
      ++timeConstrainedEnabledCount;
      if (recordConstrainedEnableInCallbackOrder) {
        callbackOrder.push_back('C');
      }
      callbackChanged.notify_all();
    }

    void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
      std::scoped_lock lock(callbackMutex);
      ++timeAdvanceGrantCount;
      callbackOrder.push_back('G');
      if (auto const* integerTime =
              dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
        timeAdvanceGrantValue = integerTime->getTime();
      }
      callbackChanged.notify_all();
    }

    bool waitForDiscovery() {
      std::unique_lock lock(callbackMutex);
      return callbackChanged.wait_for(
          lock, std::chrono::seconds(2), [this] { return discovered; });
    }

    bool waitForReflectionAndGrant(std::size_t requiredGrantCount = 1U) {
      std::unique_lock lock(callbackMutex);
      return callbackChanged.wait_for(
          lock, std::chrono::seconds(2), [this, requiredGrantCount] {
            return reflectedCount == 1U &&
                timeAdvanceGrantCount == requiredGrantCount;
      });
    }

    std::mutex callbackMutex;
    std::condition_variable callbackChanged;
    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    std::size_t reflectedCount = 0U;
    rti1516_2025::ObjectInstanceHandle reflectedObjectInstance;
    std::size_t reflectedAttributeCount = 0U;
    std::vector<std::uint8_t> reflectedValue;
    std::vector<std::uint8_t> reflectedTag;
    bool hasRetraction = false;
    rti1516_2025::OrderType sentOrderType = rti1516_2025::RECEIVE;
    rti1516_2025::OrderType receivedOrderType = rti1516_2025::RECEIVE;
    std::int64_t reflectedTimestampValue = -1;
    std::size_t timeRegulationEnabledCount = 0U;
    std::size_t timeConstrainedEnabledCount = 0U;
    bool recordConstrainedEnableInCallbackOrder = false;
    std::size_t timeAdvanceGrantCount = 0U;
    std::int64_t timeAdvanceGrantValue = -1;
    std::vector<char> callbackOrder;
  } senderFederate, receiverFederate;
  receiverFederate.recordConstrainedEnableInCallbackOrder =
      pendingTceTransition;

  constexpr wchar_t const* federationName =
      L"process-timestamped-attribute-scheduler-execution";
  constexpr wchar_t const* senderName =
      L"process-timestamped-attribute-scheduler-sender";
  constexpr wchar_t const* receiverName =
      L"process-timestamped-attribute-scheduler-receiver";
  wchar_t const* const objectClassName = regionalAttribute
                                             ? L"HLAobjectRoot.Food.Drink.Soda"
                                             : L"HLAobjectRoot.Employee";
  wchar_t const* const attributeName = regionalAttribute ? L"Flavor" : L"Name";
  wchar_t const* const dimensionName = L"SodaFlavor";

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  std::shared_ptr<ProcessTransportConnection> serverSenderConnection;
  std::shared_ptr<ProcessTransportConnection> serverReceiverConnection;
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
          {"process-timestamped-attribute-scheduler-server", 0x9310U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      serverSenderConnection = senderConnection;
      ProcessTransportSession sender(senderConnection);
      auto const senderHandler = service.handlerFor(sender);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation) {
        umbra::test::serveExpectedPrimaryProcessRequest(
            session,
            handler,
            operation,
            "The process TSO attribute scheduler received an unexpected operation.");
      };

      serveExpected(sender, senderHandler,
                    TransportServiceOperation::create_federation_execution);
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::join_federation_execution);

      auto receiverConnection = listener->accept(
          nullptr,
          {"process-timestamped-attribute-scheduler-server", 0x9311U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      serverReceiverConnection = receiverConnection;
      ProcessTransportSession receiver(receiverConnection);
      auto const receiverHandler = service.handlerFor(receiver);
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::join_federation_execution);
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::get_object_class_handle);
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::get_object_class_handle);
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::get_attribute_handle);
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::get_attribute_handle);
      if (regionalAttribute) {
        serveExpected(sender, senderHandler,
                      TransportServiceOperation::get_dimension_handle);
        serveExpected(receiver, receiverHandler,
                      TransportServiceOperation::get_dimension_handle);
        serveExpected(sender, senderHandler,
                      TransportServiceOperation::create_region);
        serveExpected(sender, senderHandler,
                      TransportServiceOperation::set_range_bounds);
        serveExpected(sender, senderHandler,
                      TransportServiceOperation::commit_region_modifications);
        serveExpected(receiver, receiverHandler,
                      TransportServiceOperation::create_region);
        serveExpected(receiver, receiverHandler,
                      TransportServiceOperation::set_range_bounds);
        serveExpected(receiver, receiverHandler,
                      TransportServiceOperation::commit_region_modifications);
        serveExpected(receiver, receiverHandler,
                      TransportServiceOperation::subscribe_object_class_attributes_with_regions);
        serveExpected(sender, senderHandler,
                      TransportServiceOperation::publish_object_class_attributes);
      } else {
        serveExpected(sender, senderHandler,
                      TransportServiceOperation::publish_object_class_attributes);
        serveExpected(receiver, receiverHandler,
                      TransportServiceOperation::subscribe_object_class_attributes);
      }
      umbra::test::serveExpectedProcessSuccessfulVoidService(
          serveExpected, sender, senderHandler,
          TransportServiceOperation::enable_time_regulation);
      if (pendingTceTransition) {
        umbra::test::serveExpectedProcessSuccessfulVoidService(
            serveExpected, receiver, receiverHandler,
            TransportServiceOperation::time_advance_request);
      }
      umbra::test::serveExpectedProcessSuccessfulVoidService(
          serveExpected, receiver, receiverHandler,
          TransportServiceOperation::enable_time_constrained);
      serveExpected(sender, senderHandler,
                    regionalAttribute
                        ? TransportServiceOperation::register_object_instance_with_regions
                        : TransportServiceOperation::register_object_instance);
      if (callbackModel == HLA_EVOKED) {
        serveExpected(receiver, receiverHandler,
                      TransportServiceOperation::receive_interaction);
      }
      if (pendingTceTransition) {
        serveExpected(sender, senderHandler,
                      TransportServiceOperation::update_attribute_values);
        umbra::test::serveExpectedProcessSuccessfulVoidService(
            serveExpected, sender, senderHandler,
            TransportServiceOperation::time_advance_request);
        umbra::test::serveExpectedProcessSuccessfulVoidService(
            serveExpected, sender, senderHandler,
            TransportServiceOperation::time_advance_request);
        umbra::test::serveExpectedProcessSuccessfulVoidService(
            serveExpected, receiver, receiverHandler,
            TransportServiceOperation::time_advance_request);
      } else if (requestKind == ProcessTimeRequest::nmra) {
        umbra::test::serveExpectedProcessSuccessfulVoidService(
            serveExpected, receiver, receiverHandler,
            TransportServiceOperation::next_message_request_available);
      } else if (requestKind == ProcessTimeRequest::nmr) {
        umbra::test::serveExpectedProcessSuccessfulVoidService(
            serveExpected, receiver, receiverHandler,
            TransportServiceOperation::next_message_request);
      } else {
        umbra::test::serveExpectedProcessSuccessfulVoidService(
            serveExpected, receiver, receiverHandler,
            TransportServiceOperation::time_advance_request);
      }
      if (!pendingTceTransition) {
        serveExpected(sender, senderHandler,
                      TransportServiceOperation::update_attribute_values);
        umbra::test::serveExpectedProcessSuccessfulVoidService(
            serveExpected, sender, senderHandler,
            TransportServiceOperation::time_advance_request);
      }
      if (callbackModel == HLA_EVOKED) {
        serveExpected(receiver, receiverHandler,
                      TransportServiceOperation::query_time_bounds);
        serveExpected(receiver, receiverHandler,
                      TransportServiceOperation::query_time_bounds);
      }
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::acknowledge_tso_delivery);
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::resign_federation_execution);
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::resign_federation_execution);
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
  auto senderConfiguration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"process-timestamped-attribute-scheduler-sender-client")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto receiverConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"process-timestamped-attribute-scheduler-receiver-client")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(
                senderFederate, callbackModel, senderConfiguration)
                .addressUsed);
    REQUIRE_NOTHROW(senderRti->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    auto const senderHandle = senderRti->joinFederationExecution(
        senderName, L"process-timestamped-attribute-scheduler-type", federationName);
    REQUIRE(senderHandle.isValid());
    senderJoined = true;
    REQUIRE(receiverRti->connect(
                receiverFederate, callbackModel, receiverConfiguration)
                .addressUsed);
    auto const receiverHandle = receiverRti->joinFederationExecution(
        receiverName, L"process-timestamped-attribute-scheduler-type", federationName);
    REQUIRE(receiverHandle.isValid());
    receiverJoined = true;

    auto const senderObjectClass = senderRti->getObjectClassHandle(objectClassName);
    auto const receiverObjectClass = receiverRti->getObjectClassHandle(objectClassName);
    auto const senderAttribute = senderRti->getAttributeHandle(
        senderObjectClass, attributeName);
    auto const receiverAttribute = receiverRti->getAttributeHandle(
        receiverObjectClass, attributeName);
    REQUIRE(senderObjectClass == receiverObjectClass);
    REQUIRE(senderAttribute == receiverAttribute);
    std::optional<rti1516_2025::RegionHandle> senderRegion;
    if (regionalAttribute) {
      auto const senderDimension = senderRti->getDimensionHandle(dimensionName);
      auto const receiverDimension = receiverRti->getDimensionHandle(dimensionName);
      senderRegion = senderRti->createRegion(
          rti1516_2025::DimensionHandleSet{senderDimension});
      REQUIRE_NOTHROW(senderRti->setRangeBounds(
          *senderRegion, senderDimension, rti1516_2025::RangeBounds(0UL, 1UL)));
      REQUIRE_NOTHROW(senderRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{*senderRegion}));
      auto const receiverRegion = receiverRti->createRegion(
          rti1516_2025::DimensionHandleSet{receiverDimension});
      REQUIRE_NOTHROW(receiverRti->setRangeBounds(
          receiverRegion, receiverDimension, rti1516_2025::RangeBounds(0UL, 1UL)));
      REQUIRE_NOTHROW(receiverRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{receiverRegion}));
      rti1516_2025::AttributeHandleSet const subscribedAttributes{receiverAttribute};
      rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const subscription{{
          subscribedAttributes,
          rti1516_2025::RegionHandleSet{receiverRegion},
      }};
      REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributesWithRegions(
          receiverObjectClass, subscription));
      REQUIRE_NOTHROW(senderRti->publishObjectClassAttributes(
          senderObjectClass, {senderAttribute}));
    } else {
      REQUIRE_NOTHROW(senderRti->publishObjectClassAttributes(
          senderObjectClass, {senderAttribute}));
      REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributes(
          receiverObjectClass, {receiverAttribute}, true));
    }

    REQUIRE_NOTHROW(senderRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    if (callbackModel == HLA_EVOKED) {
      {
        std::scoped_lock lock(senderFederate.callbackMutex);
        REQUIRE_FALSE(senderFederate.timeRegulationEnabledCount != 0U);
      }
      REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    }
    {
      std::scoped_lock lock(senderFederate.callbackMutex);
      REQUIRE(senderFederate.timeRegulationEnabledCount == 1U);
    }
    if (pendingTceTransition) {
      REQUIRE_NOTHROW(receiverRti->timeAdvanceRequest(
          rti1516_2025::HLAinteger64Time(5)));
      REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
      std::scoped_lock lock(receiverFederate.callbackMutex);
      REQUIRE(receiverFederate.timeAdvanceGrantCount == 1U);
      REQUIRE(receiverFederate.timeAdvanceGrantValue == 5);
      receiverFederate.callbackOrder.clear();
    }
    REQUIRE_NOTHROW(receiverRti->enableTimeConstrained());
    if (callbackModel == HLA_EVOKED) {
      {
        std::scoped_lock lock(receiverFederate.callbackMutex);
        REQUIRE_FALSE(receiverFederate.timeConstrainedEnabledCount != 0U);
      }
      if (!pendingTceTransition) {
        static_cast<void>(receiverRti->evokeCallback(0.0));
      }
    }
    {
      std::scoped_lock lock(receiverFederate.callbackMutex);
      REQUIRE(receiverFederate.timeConstrainedEnabledCount ==
              (pendingTceTransition ? 0U : 1U));
    }

    rti1516_2025::ObjectInstanceHandle objectInstance;
    if (regionalAttribute) {
      rti1516_2025::AttributeHandleSet const registeredAttributes{senderAttribute};
      rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const registration{{
          registeredAttributes,
          rti1516_2025::RegionHandleSet{*senderRegion},
      }};
      objectInstance = senderRti->registerObjectInstanceWithRegions(
          senderObjectClass, registration);
    } else {
      objectInstance = senderRti->registerObjectInstance(senderObjectClass);
    }
    REQUIRE(objectInstance.isValid());
    if (callbackModel == HLA_IMMEDIATE) {
      REQUIRE(receiverFederate.waitForDiscovery());
    } else {
      {
        std::scoped_lock lock(receiverFederate.callbackMutex);
        REQUIRE_FALSE(receiverFederate.discovered);
      }
      for (std::size_t evokeCount = 0U; evokeCount < 4U; ++evokeCount) {
        {
          std::scoped_lock lock(receiverFederate.callbackMutex);
          if (receiverFederate.discovered) {
            break;
          }
        }
        static_cast<void>(receiverRti->evokeCallback(0.0));
      }
    }
    {
      std::scoped_lock lock(receiverFederate.callbackMutex);
      REQUIRE(receiverFederate.discovered);
      REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);
    }

    if (!pendingTceTransition) {
      if (requestKind == ProcessTimeRequest::nmra) {
        REQUIRE_NOTHROW(receiverRti->nextMessageRequestAvailable(
            rti1516_2025::HLAinteger64Time(10)));
        if (callbackModel == HLA_EVOKED) {
          REQUIRE_NOTHROW(receiverRti->disableCallbacks());
        }
      } else if (requestKind == ProcessTimeRequest::nmr) {
        REQUIRE_NOTHROW(receiverRti->nextMessageRequest(
            rti1516_2025::HLAinteger64Time(10)));
      } else {
        REQUIRE_NOTHROW(receiverRti->timeAdvanceRequest(
            rti1516_2025::HLAinteger64Time(5)));
      }
      std::scoped_lock lock(receiverFederate.callbackMutex);
      REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);
    } else {
      std::scoped_lock lock(receiverFederate.callbackMutex);
      REQUIRE(receiverFederate.timeAdvanceGrantCount == 1U);
    }

    std::array<std::uint8_t, 3U> const valueBytes{0x54U, 0x53U, 0x4FU};
    std::array<std::uint8_t, 3U> const tagBytes{0x54U, 0x41U, 0x47U};
    AttributeHandleValueMap attributeValues;
    attributeValues.emplace(
        senderAttribute,
        VariableLengthData(valueBytes.data(), valueBytes.size()));
    VariableLengthData tag(tagBytes.data(), tagBytes.size());
    auto const retraction = senderRti->updateAttributeValues(
        objectInstance,
        attributeValues,
        tag,
        rti1516_2025::HLAinteger64Time(
            pendingTceTransition ? 6 : 5));
    REQUIRE(retraction.isValid());

    if (pendingTceTransition) {
      {
        std::scoped_lock lock(receiverFederate.callbackMutex);
        REQUIRE(receiverFederate.timeConstrainedEnabledCount == 0U);
        REQUIRE(receiverFederate.reflectedCount == 0U);
      }
      REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
          rti1516_2025::HLAinteger64Time(4)));
      REQUIRE_FALSE(senderRti->evokeCallback(0.0));
      {
        std::scoped_lock lock(senderFederate.callbackMutex);
        REQUIRE(senderFederate.timeAdvanceGrantCount == 1U);
        REQUIRE(senderFederate.timeAdvanceGrantValue == 4);
      }
      {
        std::scoped_lock lock(receiverFederate.callbackMutex);
        REQUIRE(receiverFederate.timeConstrainedEnabledCount == 0U);
        REQUIRE(receiverFederate.reflectedCount == 0U);
      }
      REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
          rti1516_2025::HLAinteger64Time(5)));
      REQUIRE_FALSE(senderRti->evokeCallback(0.0));
      {
        std::scoped_lock lock(senderFederate.callbackMutex);
        REQUIRE(senderFederate.timeAdvanceGrantCount == 2U);
        REQUIRE(senderFederate.timeAdvanceGrantValue == 5);
      }
      static_cast<void>(receiverRti->evokeCallback(0.0));
      {
        std::scoped_lock lock(receiverFederate.callbackMutex);
        REQUIRE(receiverFederate.timeConstrainedEnabledCount == 1U);
        REQUIRE(receiverFederate.reflectedCount == 0U);
      }
      REQUIRE_NOTHROW(receiverRti->timeAdvanceRequest(
          rti1516_2025::HLAinteger64Time(6)));
    } else {
      REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
          rti1516_2025::HLAinteger64Time(6)));
      if (callbackModel == HLA_EVOKED) {
        {
          std::scoped_lock lock(senderFederate.callbackMutex);
          REQUIRE(senderFederate.timeAdvanceGrantCount == 0U);
        }
        REQUIRE_FALSE(senderRti->evokeCallback(0.0));
      }
      {
        std::scoped_lock lock(senderFederate.callbackMutex);
        REQUIRE(senderFederate.timeAdvanceGrantCount == 1U);
        REQUIRE(senderFederate.timeAdvanceGrantValue == 6);
      }
    }
    // In the ordinary lane the producer advances to 6 while the constrained
    // receiver has TAR(5) pending. In the transition lane it advances only to
    // 5, enabling Time Constrained before the receiver requests TAR(6).
    if (callbackModel == HLA_EVOKED) {
      // The attribute event has crossed transport but remains in the
      // coordinator's inTransit phase until the public reflection callback
      // completes. Query both bounds before that callback to cover this
      // deterministic process boundary.
      {
        std::scoped_lock lock(receiverFederate.callbackMutex);
        REQUIRE(receiverFederate.reflectedCount == 0U);
      }
      rti1516_2025::HLAinteger64Time inTransitGaltValue(99);
      REQUIRE(receiverRti->queryGALT(inTransitGaltValue));
      REQUIRE(inTransitGaltValue.getTime() ==
              (pendingTceTransition ? 6 : 5));
      rti1516_2025::HLAinteger64Time inTransitLitsValue(101);
      REQUIRE(receiverRti->queryLITS(inTransitLitsValue));
      REQUIRE(inTransitLitsValue.getTime() ==
              (pendingTceTransition ? 6 : 5));
      {
        std::scoped_lock lock(receiverFederate.callbackMutex);
        REQUIRE(receiverFederate.timeAdvanceGrantCount ==
                (pendingTceTransition ? 1U : 0U));
      }
      if (requestKind == ProcessTimeRequest::nmra) {
        REQUIRE(receiverRti->evokeCallback(0.0));
        {
          std::scoped_lock lock(receiverFederate.callbackMutex);
          REQUIRE(receiverFederate.reflectedCount == 0U);
          REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);
        }
        REQUIRE_NOTHROW(receiverRti->enableCallbacks());
      }
      static_cast<void>(receiverRti->evokeCallback(0.0));
    } else {
      // The process reader delivers both the TSO reflection and the grant
      // asynchronously; no benign RTI service is used to poll the endpoint.
      REQUIRE(receiverFederate.waitForReflectionAndGrant(
          pendingTceTransition ? 2U : 1U));
    }
    if (callbackModel == HLA_EVOKED) {
      {
        std::scoped_lock lock(receiverFederate.callbackMutex);
        REQUIRE(receiverFederate.reflectedCount == 1U);
        REQUIRE(receiverFederate.timeAdvanceGrantCount ==
                (pendingTceTransition ? 1U : 0U));
        REQUIRE(receiverFederate.callbackOrder ==
                (pendingTceTransition ? std::vector<char>{'C', 'A'}
                                      : std::vector<char>{'A'}));
      }
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    {
      std::scoped_lock lock(receiverFederate.callbackMutex);
      REQUIRE(receiverFederate.reflectedCount == 1U);
      // The timestamp-6 reflection is delivered before the pending TAR(6)
      // grant. Once that message has been delivered, the inclusive GALT
      // boundary permits the receiver to grant at 6.
      REQUIRE(receiverFederate.timeAdvanceGrantCount ==
              (pendingTceTransition ? 2U : 1U));
      REQUIRE(receiverFederate.timeAdvanceGrantValue ==
              (pendingTceTransition ? 6 : 5));
      REQUIRE(receiverFederate.callbackOrder ==
              (pendingTceTransition ? std::vector<char>{'C', 'A', 'G'}
                                    : std::vector<char>{'A', 'G'}));
      REQUIRE(receiverFederate.reflectedObjectInstance == objectInstance);
      REQUIRE(receiverFederate.reflectedAttributeCount == 1U);
      REQUIRE(receiverFederate.reflectedValue ==
              std::vector<std::uint8_t>{0x54U, 0x53U, 0x4FU});
      REQUIRE(receiverFederate.reflectedTag ==
              std::vector<std::uint8_t>{0x54U, 0x41U, 0x47U});
      REQUIRE(receiverFederate.hasRetraction);
      REQUIRE(receiverFederate.sentOrderType == rti1516_2025::TIMESTAMP);
      REQUIRE(receiverFederate.receivedOrderType == rti1516_2025::TIMESTAMP);
      REQUIRE(receiverFederate.reflectedTimestampValue ==
              (pendingTceTransition ? 6 : 5));
    }

    REQUIRE_NOTHROW(senderRti->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
    senderJoined = false;
    REQUIRE_NOTHROW(receiverRti->resignFederationExecution(NO_ACTION));
    receiverJoined = false;
    senderRti->disconnect();
    receiverRti->disconnect();
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
        senderRti->resignFederationExecution(
            rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
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
}

TEST_CASE(
    "RTIambassadors deliver a deferred timestamped process attribute update before the grant",
    "[integration][foundation][object-management][time-management][time-advance][transport][process-boundary][public-endpoint][multi-federate][timestamped-process-attribute-update][process-tso-attribute-before-grant][galt][lits][query-galt-lits][rti.service.update-attribute-values][rti.service.enable-time-regulation][rti.service.enable-time-constrained][rti.service.time-advance-request][rti.service.query-galt][rti.service.query-lits][federate.callback.reflect-attribute-values][federate.callback.time-advance-grant]") {
  verifyProcessTsoAttributeBeforeGrant(ProcessTimeRequest::tar);
}

TEST_CASE(
    "RTIambassadors keep a process attribute update timestamped while Time Constrained is pending",
    "[integration][foundation][object-management][time-management][time-advance][transport][process-boundary][public-endpoint][multi-federate][timestamped-process-attribute-update][tce-transition-stored-tso-attribute][lane.tce-transition-stored-tso-attribute-process][rti.service.enable-time-constrained][rti.service.update-attribute-values][rti.service.time-advance-request][federate.callback.time-constrained-enabled][federate.callback.reflect-attribute-values][federate.callback.time-advance-grant][2025]") {
  verifyProcessTsoAttributeBeforeGrant(
      ProcessTimeRequest::tar, false, HLA_EVOKED, true);
}

TEST_CASE(
    "RTIambassadors retarget a pending process NMRA to a timestamped attribute update while callbacks are disabled",
    "[integration][foundation][object-management][time-management][time-advance][transport][process-boundary][public-endpoint][multi-federate][timestamped-process-attribute-update][process-tso-attribute-update-callback-gating][rti.service.update-attribute-values][rti.service.next-message-request-available][rti.service.disable-callbacks][rti.service.enable-callbacks][rti.service.evoke-callback][federate.callback.reflect-attribute-values][federate.callback.time-advance-grant][2025]") {
  verifyProcessTsoAttributeBeforeGrant(ProcessTimeRequest::nmra);
}

TEST_CASE(
    "RTIambassadors retarget a pending process NMR to a future timestamped attribute update",
    "[integration][foundation][object-management][time-management][time-advance][transport][process-boundary][public-endpoint][multi-federate][timestamped-process-attribute-update][process-tso-future-attribute-nmr][process-time-advance-next-message-queued-focused][rti.service.update-attribute-values][rti.service.next-message-request][rti.service.evoke-callback][rti.service.query-galt][rti.service.query-lits][federate.callback.reflect-attribute-values][federate.callback.time-advance-grant][2025]") {
  verifyProcessTsoAttributeBeforeGrant(ProcessTimeRequest::nmr);
}

TEST_CASE(
    "RTIambassadors retarget pending process NMR and NMRA to a future timestamped attribute update under HLA_IMMEDIATE",
    "[integration][foundation][object-management][time-management][time-advance][transport][process-boundary][public-endpoint][multi-federate][timestamped-process-attribute-update][process-tso-future-attribute-nmr-nmra-immediate][process-tso-immediate-no-follow-up-poll][process-time-advance-next-message-queued-focused][rti.service.update-attribute-values][rti.service.next-message-request][rti.service.next-message-request-available][federate.callback.reflect-attribute-values][federate.callback.time-advance-grant][HLA_IMMEDIATE][2025]") {
  SECTION("NMR") {
    verifyProcessTsoAttributeBeforeGrant(
        ProcessTimeRequest::nmr, false, HLA_IMMEDIATE);
  }
  SECTION("NMRA") {
    verifyProcessTsoAttributeBeforeGrant(
        ProcessTimeRequest::nmra, false, HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "RTIambassadors retarget a pending process NMR to a future timestamped regional attribute update",
    "[integration][foundation][data-distribution-management][object-management][time-management][time-advance][transport][process-boundary][public-endpoint][multi-federate][timestamped-process-regional-attribute-update][process-tso-future-regional-attribute-nmr][process-time-advance-next-message-queued-focused][rti.service.subscribe-object-class-attributes-with-regions][rti.service.register-object-instance-with-regions][rti.service.update-attribute-values][rti.service.next-message-request][federate.callback.reflect-attribute-values][federate.callback.time-advance-grant][2025]") {
  verifyProcessTsoAttributeBeforeGrant(ProcessTimeRequest::nmr, true);
}

TEST_CASE(
    "RTIambassadors deliver a future timestamped regional attribute update under HLA_IMMEDIATE without a follow-up poll",
    "[integration][foundation][data-distribution-management][object-management][time-management][time-advance][transport][process-boundary][public-endpoint][multi-federate][timestamped-process-regional-attribute-update][process-tso-immediate-regional-attribute-no-follow-up-poll][process-tso-immediate-no-follow-up-poll][process-time-advance-next-message-queued-focused][rti.service.subscribe-object-class-attributes-with-regions][rti.service.register-object-instance-with-regions][rti.service.update-attribute-values][rti.service.next-message-request][rti.service.next-message-request-available][federate.callback.reflect-attribute-values][federate.callback.time-advance-grant][HLA_IMMEDIATE][2025]") {
  SECTION("NMR") {
    verifyProcessTsoAttributeBeforeGrant(
        ProcessTimeRequest::nmr, true, HLA_IMMEDIATE);
  }
  SECTION("NMRA") {
    verifyProcessTsoAttributeBeforeGrant(
        ProcessTimeRequest::nmra, true, HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "RTIambassadors retarget a pending process NMRA to a future timestamped regional attribute update",
    "[integration][foundation][data-distribution-management][object-management][time-management][time-advance][transport][process-boundary][public-endpoint][multi-federate][timestamped-process-regional-attribute-update][process-tso-future-regional-attribute-nmra][process-time-advance-next-message-queued-focused][rti.service.subscribe-object-class-attributes-with-regions][rti.service.register-object-instance-with-regions][rti.service.update-attribute-values][rti.service.next-message-request-available][rti.service.disable-callbacks][rti.service.enable-callbacks][rti.service.evoke-callback][federate.callback.reflect-attribute-values][federate.callback.time-advance-grant][2025]") {
  verifyProcessTsoAttributeBeforeGrant(ProcessTimeRequest::nmra, true);
}

#endif

#endif
