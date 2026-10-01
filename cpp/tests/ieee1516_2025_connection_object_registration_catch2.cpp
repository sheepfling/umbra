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
    "RTIambassador publishes and unpublishes object-class attributes and registers an object through a configured process endpoint",
    "[integration][foundation][declaration-management][object-management][time-management][transport][process-boundary][public-endpoint][order-type-control][process-unpublish-object-class-attributes][rti.service.publish-object-class-attributes][rti.service.unpublish-object-class-attributes][rti.service.unpublish-object-class][rti.service.register-object-instance][rti.service.change-default-attribute-order-type][rti.service.change-attribute-order-type]") {
  auto runScenario = [](CallbackModel callbackModel) {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-object-registration-execution";
  constexpr char const* objectName = "HLAobjectRoot.Customer";
  constexpr char const* attributeName = "HLAprivilegeToDeleteObject";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto connection = listener->accept(
          nullptr,
          {"public-process-object-registration-server", 0x9601U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return umbra::test::servePrimaryProcessRequest(
            session, handler,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The public process object-registration server received an unexpected operation.");
              }
              return handler(request);
            });
      };

      if (!serveExpected(TransportServiceOperation::create_federation_execution)) {
        throw std::runtime_error(
            "The public process object-registration server lost Create.");
      }
      auto const objectClass = registry.objectClassHandleFor(federationName, objectName);
      auto const attribute = registry.attributeHandleFor(
          federationName, objectName, attributeName);
      if (!objectClass || !attribute) {
        throw std::runtime_error(
            "The public process object-registration server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);

      if (!serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::get_object_class_handle) ||
          !serveExpected(TransportServiceOperation::get_attribute_handle) ||
          !serveExpected(TransportServiceOperation::publish_object_class_attributes)) {
        throw std::runtime_error(
            "The public process object-registration server lost Join, lookup, or publication.");
      }
      auto const member = registry.memberByName(
          federationName, L"public-process-object-registration-federate");
      if (!member) {
        throw std::runtime_error(
            "The public process object-registration server could not resolve its federate.");
      }
      auto const published = registry.publishedObjectClassAttributeHandles(
          federationName, member->id, *objectClass);
      if (!published || !published->contains(*attribute)) {
        throw std::runtime_error(
            "The public process object-registration publication did not reach the registry.");
      }

      if (!serveExpected(
              TransportServiceOperation::unpublish_object_class_attributes)) {
        throw std::runtime_error(
            "The public process object-registration server lost subset unpublication.");
      }
      auto const afterSubsetUnpublish = registry.publishedObjectClassAttributeHandles(
          federationName, member->id, *objectClass);
      if (!afterSubsetUnpublish || afterSubsetUnpublish->contains(*attribute)) {
        throw std::runtime_error(
            "The public process object-registration subset unpublication did not reach the registry.");
      }

      if (!serveExpected(TransportServiceOperation::publish_object_class_attributes)) {
        throw std::runtime_error(
            "The public process object-registration server lost the publication restore.");
      }
      auto const republished = registry.publishedObjectClassAttributeHandles(
          federationName, member->id, *objectClass);
      if (!republished || !republished->contains(*attribute)) {
        throw std::runtime_error(
            "The public process object-registration publication restore did not reach the registry.");
      }

      if (!serveExpected(TransportServiceOperation::unpublish_object_class)) {
        throw std::runtime_error(
            "The public process object-registration server lost whole-class unpublication.");
      }
      auto const afterWholeUnpublish = registry.publishedObjectClassAttributeHandles(
          federationName, member->id, *objectClass);
      if (!afterWholeUnpublish || !afterWholeUnpublish->empty()) {
        throw std::runtime_error(
            "The public process object-registration whole-class unpublication did not reach the registry.");
      }

      if (!serveExpected(TransportServiceOperation::publish_object_class_attributes)) {
        throw std::runtime_error(
            "The public process object-registration server lost the final publication restore.");
      }

      if (!serveExpected(
              TransportServiceOperation::change_default_attribute_order_type) ||
          !serveExpected(TransportServiceOperation::register_object_instance) ||
          !serveExpected(TransportServiceOperation::change_attribute_order_type) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The public process object-registration server lost registration or Resign.");
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"public-process-object-registration-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool clientJoined = false;
  std::optional<rti1516_2025::FederateHandle> joinedHandle;
  try {
    auto const connectionResult = rti->connect(federate, callbackModel, configuration);
    REQUIRE(connectionResult.addressUsed);
    rti->createFederationExecution(
        federationName, L"server-owned-fom.xml");
    joinedHandle = rti->joinFederationExecution(
        L"public-process-object-registration-federate",
        L"public-process-object-registration-type",
        federationName);
    clientJoined = true;

    auto const objectClass = rti->getObjectClassHandle(
        L"HLAobjectRoot.Customer");
    REQUIRE(objectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(
                    expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    auto const attribute = rti->getAttributeHandle(
        objectClass, L"HLAprivilegeToDeleteObject");
    REQUIRE(attribute.isValid());
    rti1516_2025::AttributeHandleSet attributes;
    attributes.insert(attribute);
    rti->publishObjectClassAttributes(objectClass, attributes);
    REQUIRE_NOTHROW(rti->unpublishObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(rti->publishObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(rti->unpublishObjectClass(objectClass));
    REQUIRE_NOTHROW(rti->publishObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(rti->changeDefaultAttributeOrderType(
        objectClass, attributes, rti1516_2025::TIMESTAMP));
    auto const objectInstance = rti->registerObjectInstance(objectClass);
    REQUIRE(objectInstance.isValid());
    REQUIRE_FALSE(objectInstance.toString().empty());
    REQUIRE_NOTHROW(rti->changeAttributeOrderType(
        objectInstance, attributes, rti1516_2025::TIMESTAMP));
    rti->resignFederationExecution(NO_ACTION);
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (clientJoined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
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
  REQUIRE(joinedHandle.has_value());
  REQUIRE(joinedHandle->isValid());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "RTIambassador routes Change Default Attribute Transportation Type through a configured process endpoint",
    "[integration][foundation][declaration-management][object-management][transportation][transport][process-boundary][public-endpoint][transportation-management][process-transportation-type-control][rti.service.get-object-class-handle][rti.service.get-attribute-handle][rti.service.get-transportation-type-handle][rti.service.publish-object-class-attributes][rti.service.change-default-attribute-transportation-type][rti.service.register-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    constexpr wchar_t const* federationName =
        L"public-process-transportation-default-execution";
    constexpr wchar_t const* federateName =
        L"public-process-transportation-default-federate";
    constexpr char const* objectName = "HLAobjectRoot.Customer";
    constexpr char const* attributeName = "HLAprivilegeToDeleteObject";
    constexpr char const* transportationTypeName = "HLAreliable";
    std::atomic_uint64_t expectedObjectClass{0U};
    std::atomic_uint64_t expectedAttribute{0U};
    std::atomic_uint64_t expectedTransportation{0U};
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
        auto connection = listener->accept(
            nullptr,
            {"public-process-transportation-default-server", 0x9602U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession session(connection);
        auto handler = service.handlerFor(session);
        auto serveExpected = [&](TransportServiceOperation operation) {
          return umbra::test::servePrimaryProcessRequest(
              session, handler,
              [&](TransportServiceMessage const& request) {
                if (request.operation != operation) {
                  throw std::runtime_error(
                      "The public process transportation-default server received an unexpected operation.");
                }
                return handler(request);
              });
        };

        if (!serveExpected(TransportServiceOperation::create_federation_execution)) {
          throw std::runtime_error(
              "The public process transportation-default server lost Create.");
        }
        auto const objectClass = registry.objectClassHandleFor(
            federationName, objectName);
        auto const attribute = registry.attributeHandleFor(
            federationName, objectName, attributeName);
        auto const transportation = registry.transportationTypeHandleFor(
            federationName, transportationTypeName);
        if (!objectClass || !attribute || !transportation) {
          throw std::runtime_error(
              "The public process transportation-default server could not resolve its FOM handles.");
        }
        expectedObjectClass.store(*objectClass, std::memory_order_release);
        expectedAttribute.store(*attribute, std::memory_order_release);
        expectedTransportation.store(*transportation, std::memory_order_release);

        if (!serveExpected(TransportServiceOperation::join_federation_execution) ||
            !serveExpected(TransportServiceOperation::get_object_class_handle) ||
            !serveExpected(TransportServiceOperation::get_attribute_handle) ||
            !serveExpected(TransportServiceOperation::get_transportation_type_handle) ||
            !serveExpected(TransportServiceOperation::publish_object_class_attributes) ||
            !serveExpected(
                TransportServiceOperation::change_default_attribute_transportation_type) ||
            !serveExpected(TransportServiceOperation::register_object_instance) ||
            !serveExpected(TransportServiceOperation::resign_federation_execution)) {
          throw std::runtime_error(
              "The public process transportation-default server lost a declaration or transportation operation.");
        }
        service.detach(session);
        connection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    TestFederateAmbassador federate;
    auto rti = makeRti();
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-transportation-default-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool clientJoined = false;
    try {
      REQUIRE(rti->connect(federate, callbackModel, configuration).addressUsed);
      rti->createFederationExecution(federationName, L"server-owned-fom.xml");
      static_cast<void>(rti->joinFederationExecution(
          federateName,
          L"public-process-transportation-default-type",
          federationName));
      clientJoined = true;

      auto const objectClass = rti->getObjectClassHandle(L"HLAobjectRoot.Customer");
      REQUIRE(objectClass.toString() ==
              L"ObjectClassHandle(" +
                  std::to_wstring(
                      expectedObjectClass.load(std::memory_order_acquire)) +
                  L")");
      auto const attribute = rti->getAttributeHandle(
          objectClass, L"HLAprivilegeToDeleteObject");
      REQUIRE(attribute.isValid());
      auto const transportation = rti->getTransportationTypeHandle(
          L"HLAreliable");
      REQUIRE(transportation.toString() ==
              L"TransportationTypeHandle(" +
                  std::to_wstring(
                      expectedTransportation.load(std::memory_order_acquire)) +
                  L")");
      rti1516_2025::AttributeHandleSet attributes;
      attributes.insert(attribute);
      rti->publishObjectClassAttributes(objectClass, attributes);
      REQUIRE_NOTHROW(rti->changeDefaultAttributeTransportationType(
          objectClass, attributes, transportation));
      auto const objectInstance = rti->registerObjectInstance(objectClass);
      REQUIRE(objectInstance.isValid());
      rti->resignFederationExecution(NO_ACTION);
      rti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      if (clientJoined) {
        try {
          rti->resignFederationExecution(NO_ACTION);
        } catch (...) {
        }
      }
      try {
        rti->disconnect();
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
