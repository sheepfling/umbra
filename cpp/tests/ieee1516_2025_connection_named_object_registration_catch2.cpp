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
    "RTIambassador reserves and releases a name and registers a named object through a configured process endpoint",
    "[integration][foundation][declaration-management][object-management][callbacks][callback-controls][transport][process-boundary][process-object-instance-name-reservation][public-endpoint][rti.service.reserve-object-instance-name][rti.service.release-object-instance-name][rti.service.register-object-instance][rti.service.reserve-multiple-object-instance-names][rti.service.release-multiple-object-instance-names][federate.callback.object-instance-name-reservation-succeeded][federate.callback.multiple-object-instance-name-reservation-succeeded][federate.callback.multiple-object-instance-name-reservation-failed]") {
  auto runScenario = [](CallbackModel callbackModel) {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void objectInstanceNameReservationSucceeded(
        std::wstring const& objectInstanceName) override {
      succeeded = true;
      name = objectInstanceName;
    }

    void multipleObjectInstanceNameReservationSucceeded(
        std::set<std::wstring> const& objectInstanceNames) override {
      multipleSucceeded.push_back(objectInstanceNames);
    }

    void multipleObjectInstanceNameReservationFailed(
        std::set<std::wstring> const& objectInstanceNames) override {
      multipleFailed.push_back(objectInstanceNames);
    }

    bool succeeded = false;
    std::wstring name;
    std::vector<std::set<std::wstring>> multipleSucceeded;
    std::vector<std::set<std::wstring>> multipleFailed;
  } federate;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-named-registration-execution";
  constexpr wchar_t const* requestedName = L"process-named-object";
  constexpr wchar_t const* releaseName = L"process-release-object";
  std::set<std::wstring> const multipleNames{
      L"process-multiple-a", L"process-multiple-b"};
  std::set<std::wstring> const partialMultipleNames{
      L"process-multiple-b", L"process-multiple-c"};
  std::set<std::wstring> const allMultipleNames{
      L"process-multiple-a", L"process-multiple-b", L"process-multiple-c"};
  std::set<std::wstring> const partialMultipleSuccess{
      L"process-multiple-c"};
  std::set<std::wstring> const partialMultipleFailure{
      L"process-multiple-b"};
  constexpr char const* objectName = "HLAobjectRoot.Customer";
  constexpr char const* attributeName = "HLAprivilegeToDeleteObject";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::atomic_uint64_t expectedObjectInstance{0U};
  std::atomic_bool duplicateRegistrationRejected{false};
  std::atomic_bool illegalReservationRejected{false};
  std::atomic_bool reservationAccepted{false};
  std::atomic_bool releaseReservationAccepted{false};
  std::atomic_bool releaseReservationRejected{false};
  std::atomic_bool multipleReservationAccepted{false};
  std::atomic_bool multipleReservationPartial{false};
  std::atomic_bool multipleReleaseAccepted{false};
  std::atomic_bool multipleReleaseRejected{false};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          composedProcessDefinition(),
          ProcessFederationServiceOptions{callbackModel == HLA_IMMEDIATE});
      auto connection = listener->accept(
          nullptr,
          {"public-process-named-registration-server", 0x9A01U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      std::size_t registrationCount = 0U;
      std::size_t reservationCount = 0U;
      std::size_t releaseCount = 0U;
      std::size_t multipleReservationCount = 0U;
      std::size_t multipleReleaseCount = 0U;
      auto serveExpected = [&](TransportServiceOperation operation) {
        return umbra::test::servePrimaryProcessRequest(
            session, handler,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The public process named-registration server received an unexpected operation.");
              }
              auto response = handler(request);
              if (operation == TransportServiceOperation::reserve_object_instance_name &&
                  response.status == TransportServiceStatus::ok) {
                auto const result =
                    umbra::detail::decodeProcessFederationReserveObjectInstanceNameResult(
                        response.payload);
                ++reservationCount;
                if (reservationCount == 1U) {
                  reservationAccepted.store(
                      result.succeeded &&
                          result.status ==
                          umbra::detail::ObjectInstanceNameReservationStatus::applied &&
                          result.objectInstanceName == requestedName,
                      std::memory_order_release);
                } else if (reservationCount == 2U &&
                    result.status ==
                    umbra::detail::ObjectInstanceNameReservationStatus::illegal_name) {
                  illegalReservationRejected.store(
                      true, std::memory_order_release);
                } else if (reservationCount == 3U) {
                  releaseReservationAccepted.store(
                      result.succeeded &&
                          result.status ==
                              umbra::detail::ObjectInstanceNameReservationStatus::applied &&
                          result.objectInstanceName == releaseName,
                      std::memory_order_release);
                } else {
                  throw std::runtime_error(
                      "The process service did not report illegal reservation input.");
                }
              }
              if (operation == TransportServiceOperation::register_object_instance &&
                  response.status == TransportServiceStatus::ok) {
                auto const result =
                    umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                        response.payload);
                ++registrationCount;
                if (registrationCount == 1U) {
                  expectedObjectInstance.store(
                      result.objectInstanceHandle, std::memory_order_release);
                  if (result.status !=
                          umbra::detail::ObjectInstanceRegistrationStatus::applied ||
                      result.objectInstanceName != requestedName) {
                    throw std::runtime_error(
                        "The process service did not preserve the reserved object-instance name.");
                  }
                } else if (
                    result.status ==
                    umbra::detail::ObjectInstanceRegistrationStatus::object_instance_name_in_use) {
                  duplicateRegistrationRejected.store(
                      true, std::memory_order_release);
                } else {
                  throw std::runtime_error(
                      "The process service did not report duplicate named registration as in use.");
                }
              }
              if (operation == TransportServiceOperation::release_object_instance_name &&
                  response.status == TransportServiceStatus::ok) {
                auto const result =
                    umbra::detail::decodeProcessFederationObjectInstanceNameReleaseResult(
                        response.payload);
                ++releaseCount;
                if (releaseCount == 1U) {
                  if (result.status !=
                      umbra::detail::ObjectInstanceNameReservationStatus::applied) {
                    throw std::runtime_error(
                        "The process service did not apply the reserved-name release.");
                  }
                } else if (
                    releaseCount == 2U &&
                    result.status ==
                        umbra::detail::ObjectInstanceNameReservationStatus::object_instance_name_not_reserved) {
                  releaseReservationRejected.store(true, std::memory_order_release);
                } else {
                  throw std::runtime_error(
                      "The process service did not report the second release as unreserved.");
                }
              }
              if (operation ==
                      TransportServiceOperation::reserve_multiple_object_instance_names &&
                  response.status == TransportServiceStatus::ok) {
                auto const result =
                    umbra::detail::decodeProcessFederationReserveMultipleObjectInstanceNamesResult(
                        response.payload);
                ++multipleReservationCount;
                if (multipleReservationCount == 1U) {
                  multipleReservationAccepted.store(
                      result.status ==
                              umbra::detail::ObjectInstanceNameReservationStatus::applied &&
                          result.succeededNames == multipleNames &&
                          result.failedNames.empty(),
                      std::memory_order_release);
                } else if (multipleReservationCount == 2U) {
                  multipleReservationPartial.store(
                      result.status ==
                              umbra::detail::ObjectInstanceNameReservationStatus::applied &&
                          result.succeededNames == partialMultipleSuccess &&
                          result.failedNames == partialMultipleFailure,
                      std::memory_order_release);
                } else {
                  throw std::runtime_error(
                      "The process service received an unexpected multiple reservation.");
                }
              }
              if (operation ==
                      TransportServiceOperation::release_multiple_object_instance_names &&
                  response.status == TransportServiceStatus::ok) {
                auto const result =
                    umbra::detail::decodeProcessFederationReleaseMultipleObjectInstanceNamesResult(
                        response.payload);
                ++multipleReleaseCount;
                if (multipleReleaseCount == 1U) {
                  if (result.status !=
                      umbra::detail::ObjectInstanceNameReservationStatus::applied) {
                    throw std::runtime_error(
                        "The process service did not apply the multiple reserved-name release.");
                  }
                  multipleReleaseAccepted.store(true, std::memory_order_release);
                } else if (
                    multipleReleaseCount == 2U &&
                    result.status ==
                        umbra::detail::ObjectInstanceNameReservationStatus::object_instance_name_not_reserved) {
                  multipleReleaseRejected.store(true, std::memory_order_release);
                } else {
                  throw std::runtime_error(
                      "The process service did not report the repeated multiple release as unreserved.");
                }
              }
              return response;
            });
      };

      if (!serveExpected(TransportServiceOperation::create_federation_execution)) {
        throw std::runtime_error(
            "The public process named-registration server lost Create.");
      }
      auto const objectClass = registry.objectClassHandleFor(federationName, objectName);
      auto const attribute = registry.attributeHandleFor(
          federationName, objectName, attributeName);
      if (!objectClass || !attribute) {
        throw std::runtime_error(
            "The public process named-registration server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      if (!serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::get_object_class_handle) ||
          !serveExpected(TransportServiceOperation::get_attribute_handle) ||
          !serveExpected(TransportServiceOperation::publish_object_class_attributes) ||
          !serveExpected(TransportServiceOperation::reserve_object_instance_name) ||
          (callbackModel == HLA_IMMEDIATE &&
           !serveExpected(TransportServiceOperation::get_object_class_handle)) ||
          !serveExpected(TransportServiceOperation::register_object_instance) ||
          !serveExpected(TransportServiceOperation::register_object_instance) ||
          !serveExpected(TransportServiceOperation::report_failed_service_invocation) ||
          !serveExpected(TransportServiceOperation::reserve_object_instance_name) ||
          !serveExpected(TransportServiceOperation::report_failed_service_invocation) ||
          !serveExpected(TransportServiceOperation::reserve_object_instance_name) ||
          (callbackModel == HLA_IMMEDIATE &&
           !serveExpected(TransportServiceOperation::get_object_class_handle)) ||
          !serveExpected(TransportServiceOperation::release_object_instance_name) ||
          !serveExpected(TransportServiceOperation::release_object_instance_name) ||
          !serveExpected(TransportServiceOperation::report_failed_service_invocation) ||
          !serveExpected(
              TransportServiceOperation::reserve_multiple_object_instance_names) ||
          (callbackModel == HLA_IMMEDIATE &&
           !serveExpected(TransportServiceOperation::get_object_class_handle)) ||
          !serveExpected(
              TransportServiceOperation::reserve_multiple_object_instance_names) ||
          (callbackModel == HLA_IMMEDIATE &&
           !serveExpected(TransportServiceOperation::get_object_class_handle)) ||
          !serveExpected(
              TransportServiceOperation::release_multiple_object_instance_names) ||
          !serveExpected(
              TransportServiceOperation::release_multiple_object_instance_names) ||
          !serveExpected(TransportServiceOperation::report_failed_service_invocation) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The public process named-registration server lost a required operation.");
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"public-process-named-registration-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool clientJoined = false;
  try {
    REQUIRE(rti->connect(federate, callbackModel, configuration).addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    rti->joinFederationExecution(
        L"public-process-named-registration-federate",
        L"public-process-named-registration-type",
        federationName);
    clientJoined = true;

    auto const objectClass = rti->getObjectClassHandle(L"HLAobjectRoot.Customer");
    REQUIRE(objectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    auto const attribute = rti->getAttributeHandle(
        objectClass, L"HLAprivilegeToDeleteObject");
    REQUIRE(attribute == rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                             expectedAttribute.load(std::memory_order_acquire)));
    rti1516_2025::AttributeHandleSet attributes;
    attributes.insert(attribute);
    rti->publishObjectClassAttributes(objectClass, attributes);

    rti->reserveObjectInstanceName(requestedName);
    if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(rti->getObjectClassHandle(L"HLAobjectRoot.Customer"));
    } else {
      REQUIRE_FALSE(federate.succeeded);
      static_cast<void>(rti->evokeCallback(0.0));
    }
    REQUIRE(federate.succeeded);
    REQUIRE(federate.name == requestedName);

    auto const objectInstance = rti->registerObjectInstance(objectClass, requestedName);
    REQUIRE(objectInstance.isValid());
    REQUIRE(objectInstance.toString() ==
            L"ObjectInstanceHandle(" +
                std::to_wstring(expectedObjectInstance.load(std::memory_order_acquire)) +
                L")");
    REQUIRE_THROWS_AS(
        rti->registerObjectInstance(objectClass, requestedName),
        rti1516_2025::ObjectInstanceNameInUse);
    REQUIRE_THROWS_AS(
        rti->reserveObjectInstanceName(L"HLA.illegal-process-name"),
        rti1516_2025::IllegalName);
    rti->reserveObjectInstanceName(releaseName);
    if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(rti->getObjectClassHandle(L"HLAobjectRoot.Customer"));
    } else {
      static_cast<void>(rti->evokeCallback(0.0));
    }
    REQUIRE(federate.succeeded);
    REQUIRE(federate.name == releaseName);
    REQUIRE_NOTHROW(rti->releaseObjectInstanceName(releaseName));
    REQUIRE_THROWS_AS(
        rti->releaseObjectInstanceName(releaseName),
        rti1516_2025::ObjectInstanceNameNotReserved);

    REQUIRE_NOTHROW(rti->reserveMultipleObjectInstanceNames(multipleNames));
    if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(rti->getObjectClassHandle(L"HLAobjectRoot.Customer"));
    } else {
      static_cast<void>(rti->evokeCallback(0.0));
    }
    REQUIRE(federate.multipleSucceeded.size() == 1U);
    REQUIRE(federate.multipleSucceeded.front() == multipleNames);
    REQUIRE(federate.multipleFailed.empty());

    REQUIRE_NOTHROW(rti->reserveMultipleObjectInstanceNames(partialMultipleNames));
    if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(rti->getObjectClassHandle(L"HLAobjectRoot.Customer"));
    } else {
      for (std::size_t callbackCount = 0U;
           callbackCount < 2U &&
           (federate.multipleSucceeded.size() < 2U ||
            federate.multipleFailed.empty());
           ++callbackCount) {
        static_cast<void>(rti->evokeCallback(0.0));
      }
    }
    REQUIRE(federate.multipleSucceeded.size() == 2U);
    REQUIRE(federate.multipleSucceeded.back() == partialMultipleSuccess);
    REQUIRE(federate.multipleFailed.size() == 1U);
    REQUIRE(federate.multipleFailed.front() == partialMultipleFailure);

    REQUIRE_NOTHROW(rti->releaseMultipleObjectInstanceNames(allMultipleNames));
    REQUIRE_THROWS_AS(
        rti->releaseMultipleObjectInstanceNames(allMultipleNames),
        rti1516_2025::ObjectInstanceNameNotReserved);
    rti->resignFederationExecution(NO_ACTION);
    clientJoined = false;
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
  REQUIRE(reservationAccepted.load(std::memory_order_acquire));
  REQUIRE(releaseReservationAccepted.load(std::memory_order_acquire));
  REQUIRE(expectedObjectInstance.load(std::memory_order_acquire) != 0U);
  REQUIRE(duplicateRegistrationRejected.load(std::memory_order_acquire));
  REQUIRE(illegalReservationRejected.load(std::memory_order_acquire));
  REQUIRE(releaseReservationRejected.load(std::memory_order_acquire));
  REQUIRE(multipleReservationAccepted.load(std::memory_order_acquire));
  REQUIRE(multipleReservationPartial.load(std::memory_order_acquire));
  REQUIRE(multipleReleaseAccepted.load(std::memory_order_acquire));
  REQUIRE(multipleReleaseRejected.load(std::memory_order_acquire));
  REQUIRE_FALSE(clientJoined);
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
