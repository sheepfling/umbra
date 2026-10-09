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
#include "ieee1516_2025_process_fom_test_support.hpp"
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
using umbra::test::process_fom_support_2025::composedDirectedProcessDefinition;
using umbra::test::process_fom_support_2025::composedProcessDefinition;
using umbra::test::process_fom_support_2025::processResourcePath;
using umbra::test::process_fom_support_2025::validatedProcessModule;

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
    "RTIambassadors suppress a retracted timestamped process attribute before the callback",
    "[integration][foundation][object-management][time-management][time-advance]"
    "[transport][process-boundary][public-endpoint][multi-federate][tso][retraction]"
    "[timestamped-process-attribute-update][process-tso-attribute-retraction-before-callback]"
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
      discoveredObjectInstance = objectInstance;
      discovered = true;
    }

    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const&,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const*,
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::OrderType sentOrder,
        rti1516_2025::OrderType receivedOrder,
        rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
      ++reflectedCount;
      reflectedObjectInstance = objectInstance;
      reflectedAttributeCount = attributeValues.size();
      reflectedProducingFederate = producingFederate;
      reflectedSentOrder = sentOrder;
      reflectedReceivedOrder = receivedOrder;
      reflectedHasRetraction = optionalRetraction != nullptr &&
          optionalRetraction->isValid();
      reflectedTimestampValue = -1;
      if (auto const* integerTime =
              dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
        reflectedTimestampValue = integerTime->getTime();
      }
      reflectedTag.clear();
      if (userSuppliedTag.size() != 0U) {
        auto const* data =
            static_cast<std::uint8_t const*>(userSuppliedTag.data());
        reflectedTag.assign(data, data + userSuppliedTag.size());
      }
    }

    void timeRegulationEnabled(rti1516_2025::LogicalTime const&) override {
      ++timeRegulationEnabledCount;
    }

    void timeConstrainedEnabled(rti1516_2025::LogicalTime const&) override {
      ++timeConstrainedEnabledCount;
    }

    void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
      ++timeAdvanceGrantCount;
      callbackOrder.push_back('G');
      if (auto const* integerTime =
              dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
        timeAdvanceGrantValue = integerTime->getTime();
      }
    }

    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    std::size_t reflectedCount = 0U;
    rti1516_2025::ObjectInstanceHandle reflectedObjectInstance;
    std::size_t reflectedAttributeCount = 0U;
    rti1516_2025::FederateHandle reflectedProducingFederate;
    rti1516_2025::OrderType reflectedSentOrder = rti1516_2025::RECEIVE;
    rti1516_2025::OrderType reflectedReceivedOrder = rti1516_2025::RECEIVE;
    bool reflectedHasRetraction = false;
    std::int64_t reflectedTimestampValue = -1;
    std::vector<std::uint8_t> reflectedTag;
    std::size_t timeRegulationEnabledCount = 0U;
    std::size_t timeConstrainedEnabledCount = 0U;
    std::size_t timeAdvanceGrantCount = 0U;
    std::int64_t timeAdvanceGrantValue = -1;
    std::vector<char> callbackOrder;
  } senderFederate, receiverFederate;

  constexpr wchar_t const* federationName =
      L"process-timestamped-attribute-retraction-execution";
  constexpr wchar_t const* senderName =
      L"process-timestamped-attribute-retraction-sender";
  constexpr wchar_t const* receiverName =
      L"process-timestamped-attribute-retraction-receiver";
  constexpr wchar_t const* objectClassName = L"HLAobjectRoot.Employee";
  constexpr wchar_t const* attributeName = L"Name";

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"process-timestamped-attribute-retraction-server", 0x9340U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
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
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };

      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::create_federation_execution,
          "The process attribute-retraction server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationName, "HLAobjectRoot.Employee");
      auto const attribute = registry.attributeHandleFor(
          federationName, "HLAobjectRoot.Employee", "Name");
      if (!objectClass || !attribute) {
        throw std::runtime_error(
            "The process attribute-retraction server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The process attribute-retraction server lost sender Join.");

      auto receiverConnection = listener->accept(
          nullptr,
          {"process-timestamped-attribute-retraction-server", 0x9341U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto const receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The process attribute-retraction server lost receiver Join.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process attribute-retraction server lost sender object lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process attribute-retraction server lost receiver object lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process attribute-retraction server lost sender attribute lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process attribute-retraction server lost receiver attribute lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process attribute-retraction server lost Publish.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The process attribute-retraction server lost Subscribe.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::enable_time_regulation,
          "The process attribute-retraction server lost Enable Time Regulation.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::enable_time_constrained,
          "The process attribute-retraction server lost Enable Time Constrained.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::register_object_instance,
          "The process attribute-retraction server lost Register.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The process attribute-retraction server lost discovery polling.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::time_advance_request,
          "The process attribute-retraction server lost receiver TAR.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::update_attribute_values,
          "The process attribute-retraction server lost Update Attribute Values.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The process attribute-retraction server lost sender intermediate TAR.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::retract,
          "The process attribute-retraction server lost Retract.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The process attribute-retraction server lost sender TAR.");
      // Retract happened before the TSO payload crossed the receiver's
      // process callback boundary.  The receiver still polls once to admit
      // its grant, but no receive-attribute-update or acknowledgement frame
      // is legal for the retracted message.
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The process attribute-retraction server lost receiver grant polling.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process attribute-retraction server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process attribute-retraction server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto senderConfiguration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"process-timestamped-attribute-retraction-sender-client")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto receiverConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"process-timestamped-attribute-retraction-receiver-client")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(
                senderFederate, HLA_EVOKED, senderConfiguration)
                .addressUsed);
    REQUIRE_NOTHROW(senderRti->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    auto const senderHandle = senderRti->joinFederationExecution(
        senderName,
        L"process-timestamped-attribute-retraction-type",
        federationName);
    REQUIRE(senderHandle.isValid());
    senderJoined = true;
    REQUIRE(receiverRti->connect(
                receiverFederate, HLA_EVOKED, receiverConfiguration)
                .addressUsed);
    auto const receiverHandle = receiverRti->joinFederationExecution(
        receiverName,
        L"process-timestamped-attribute-retraction-type",
        federationName);
    REQUIRE(receiverHandle.isValid());
    receiverJoined = true;

    auto const senderObjectClass = senderRti->getObjectClassHandle(objectClassName);
    auto const receiverObjectClass = receiverRti->getObjectClassHandle(objectClassName);
    auto const senderAttribute = senderRti->getAttributeHandle(
        senderObjectClass, attributeName);
    auto const receiverAttribute = receiverRti->getAttributeHandle(
        receiverObjectClass, attributeName);
    REQUIRE(senderObjectClass == receiverObjectClass);
    REQUIRE(senderObjectClass ==
            rti1516_2025::umbra_binding_detail::makeObjectClassHandle(
                expectedObjectClass.load(std::memory_order_acquire)));
    REQUIRE(senderAttribute == receiverAttribute);
    REQUIRE(senderAttribute ==
            rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                expectedAttribute.load(std::memory_order_acquire)));
    REQUIRE_NOTHROW(senderRti->publishObjectClassAttributes(
        senderObjectClass, {senderAttribute}));
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributes(
        receiverObjectClass, {receiverAttribute}, true));

    REQUIRE_NOTHROW(senderRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE(senderFederate.timeRegulationEnabledCount == 0U);
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeRegulationEnabledCount == 1U);
    REQUIRE_NOTHROW(receiverRti->enableTimeConstrained());
    REQUIRE(receiverFederate.timeConstrainedEnabledCount == 0U);
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.timeConstrainedEnabledCount == 1U);

    auto const objectInstance = senderRti->registerObjectInstance(senderObjectClass);
    REQUIRE(objectInstance.isValid());
    REQUIRE_FALSE(receiverFederate.discovered);
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.discovered);
    REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);

    REQUIRE_NOTHROW(receiverRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);

    std::array<std::uint8_t, 3U> const valueBytes{0x52U, 0x45U, 0x54U};
    std::array<std::uint8_t, 3U> const tagBytes{0x42U, 0x45U, 0x46U};
    AttributeHandleValueMap attributeValues;
    attributeValues.emplace(
        senderAttribute,
        VariableLengthData(valueBytes.data(), valueBytes.size()));
    auto const retraction = senderRti->updateAttributeValues(
        objectInstance,
        attributeValues,
        VariableLengthData(tagBytes.data(), tagBytes.size()),
        rti1516_2025::HLAinteger64Time(5));
    REQUIRE(retraction.isValid());
    // Advance the producer only to an intermediate frontier before retracting.
    // Keeping the request below the update timestamp leaves the payload queued
    // while the receiver's request remains pending, matching the standard
    // retract-before-callback boundary without depending on scheduler order.
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(2)));
    REQUIRE(senderFederate.timeAdvanceGrantCount == 0U);
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(senderFederate.timeAdvanceGrantValue == 2);
    REQUIRE_NOTHROW(senderRti->retract(retraction));
    REQUIRE(receiverFederate.reflectedCount == 0U);

    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE(senderFederate.timeAdvanceGrantCount == 1U);
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeAdvanceGrantCount == 2U);
    REQUIRE(senderFederate.timeAdvanceGrantValue == 5);

    // Evoke Callback returns whether another callback remains queued after
    // this invocation; the grant is the receiver's final callback here.
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.reflectedCount == 0U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(receiverFederate.timeAdvanceGrantValue == 5);
    REQUIRE(receiverFederate.callbackOrder == std::vector<char>{'G'});

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
      FAIL_CHECK(std::string("process attribute-retraction server: ") +
                 error.what());
    } catch (...) {
      FAIL_CHECK("process attribute-retraction server failed with an unknown exception");
    }
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
}

#endif
#endif
