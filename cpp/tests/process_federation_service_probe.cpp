#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/federation/process_federation_callback_bridge.hpp"
#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_host.hpp"
#include "process_federation_service_probe_time_management.hpp"
#include "process_public_service_fixture.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/time/reference_time_selection.hpp"

#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FederationManagementCoordinator;
using umbra::detail::FederationManagementResources;
using umbra::detail::FederationRegistryStatus;
using umbra::detail::FomCompositionStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::FomValidationStatus;
using umbra::detail::InteractionClassDeclarationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::ProcessFederationClient;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceHost;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportConnection;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::ProcessTransportSession;
using umbra::detail::ReferenceLogicalTimeSelector;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceOperation;
using umbra::detail::TransportServiceStatus;
using umbra::detail::decodeProcessFederationRegisterObjectInstanceResult;

class ProbeFederateAmbassador final : public rti1516_2025::NullFederateAmbassador {
 public:
  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::ParameterHandleValueMap const& parameterValues,
      rti1516_2025::VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const& transportationType,
      rti1516_2025::FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
    received = true;
    interactionClassHandle = interactionClass;
    transportationTypeHandle = transportationType;
    producingFederateHandle = producingFederate;
    parameterCount = parameterValues.size();
    hasOptionalSentRegions = optionalSentRegions != nullptr;
    tag.clear();
    if (userSuppliedTag.size() != 0U) {
      auto const* first = static_cast<std::uint8_t const*>(userSuppliedTag.data());
      tag.assign(first, first + userSuppliedTag.size());
    }
  }

  bool received = false;
  rti1516_2025::InteractionClassHandle interactionClassHandle;
  rti1516_2025::TransportationTypeHandle transportationTypeHandle;
  rti1516_2025::FederateHandle producingFederateHandle;
  std::size_t parameterCount = 0U;
  bool hasOptionalSentRegions = false;
  std::vector<std::uint8_t> tag;
};

std::filesystem::path resourcePath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relative;
}

PrevalidatedFomModule validatedModule(
    std::filesystem::path const& source,
    FomModuleKind kind,
    std::wstring designator) {
  LibXml2FomValidator validator;
  auto result = validator.validate({
      source,
      resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      kind,
      std::move(designator),
      L"IEEE1516-DIF-2025.xsd",
  });
  if (result.status != FomValidationStatus::valid || !result.module) {
    throw std::runtime_error("The process probe FOM did not validate.");
  }
  return *result.module;
}

FederationDefinition composedRestaurantDefinition() {
  std::vector<PrevalidatedFomModule> modules{
      validatedModule(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:process-probe-mim"),
      validatedModule(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:process-probe-restaurant"),
  };
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid ||
      !result.catalog || !result.fdd) {
    throw std::runtime_error("The process probe FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

// The process endpoint owns the registry, but FOM validation/composition
// remains a standards-derived service rather than transport behavior. Keep
// the coordinator's dependencies alive alongside the service so an
// additional-FOM Join follows the same 2025 path as the embedded profile.
struct ProcessFomPreparationContext final {
  LibXml2FomValidator validator;
  LibXml2FomModuleComposer composer;
  ReferenceLogicalTimeSelector timeSelector;
  FederationManagementCoordinator coordinator;

  ProcessFomPreparationContext()
      : composer(resourcePath("schemas/IEEE1516-FDD-2025.xsd")),
        coordinator(
            validator,
            composer,
            timeSelector,
            FederationManagementResources{
                resourcePath("mim/HLAstandardMIM-2025.xml"),
                resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
                std::nullopt}) {}
};

ProcessFederationServiceOptions makeProcessServiceOptions(
    ProcessFomPreparationContext& fomContext) {
  ProcessFederationServiceOptions options;
  options.pushReceiveOrderEvents = true;
  options.createFomPreparation =
      [&fomContext](std::vector<std::wstring> const& fomModules,
                    std::optional<std::wstring> const& mimModule,
                    std::wstring const& logicalTimeImplementationName)
      -> std::optional<FederationDefinition> {
    auto prepared = fomContext.coordinator.prepareCreate(
        fomModules,
        mimModule,
        logicalTimeImplementationName);
    if (!prepared.accepted()) {
      return std::nullopt;
    }
    return std::move(prepared.definition);
  };
  options.additionalFomPreparation =
      [&fomContext](FederationDefinition const& existing,
                    std::vector<std::wstring> const& additionalModules)
      -> std::optional<FederationDefinition> {
    auto prepared = fomContext.coordinator.prepareAdditionalModules(
        existing,
        additionalModules);
    if (!prepared.accepted()) {
      return std::nullopt;
    }
    return std::move(prepared.definition);
  };
  return options;
}

void writeText(std::filesystem::path const& path, std::string const& text) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) {
    throw std::runtime_error("The process probe could not write its status file.");
  }
  output << text;
  if (!output) {
    throw std::runtime_error("The process probe could not flush its status file.");
  }
}

template <typename Reader>
auto waitForFile(
    std::filesystem::path const& path,
    Reader&& reader) {
  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::seconds(30);
  while (std::chrono::steady_clock::now() < deadline) {
    if (std::filesystem::exists(path)) {
      try {
        return reader(path);
      } catch (std::exception const&) {
        // The writer may still be flushing a newly-created marker. Retry until
        // the bounded startup deadline rather than accepting a partial value.
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  throw std::runtime_error("The process probe timed out waiting for a marker file.");
}

std::uint16_t readPort(std::filesystem::path const& path) {
  return waitForFile(path, [](std::filesystem::path const& marker) {
    std::ifstream input(marker, std::ios::binary);
    unsigned int port = 0U;
    input >> port;
    if (!input || port == 0U || port > 65535U) {
      throw std::runtime_error("The process probe port marker is invalid.");
    }
    return static_cast<std::uint16_t>(port);
  });
}

std::uint64_t readHandle(std::filesystem::path const& path) {
  return waitForFile(path, [](std::filesystem::path const& marker) {
    std::ifstream input(marker, std::ios::binary);
    std::uint64_t handle = 0U;
    input >> handle;
    if (!input || handle == 0U) {
      throw std::runtime_error("The process probe handle marker is invalid.");
    }
    return handle;
  });
}

void publishPortMarker(
    std::filesystem::path const& path,
    std::uint16_t port) {
  auto temporary = path;
  temporary += ".tmp";
  writeText(temporary, std::to_string(port));
  std::filesystem::rename(temporary, path);
}

std::string awaitProcessMarker(
    std::filesystem::path const& directory,
    std::string const& markerName,
    std::string const& errorRole,
    std::chrono::seconds timeout = std::chrono::seconds(15)) {
  auto const marker = directory / markerName;
  auto const errorMarker = directory / (errorRole + ".error");
  auto const deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (std::filesystem::exists(errorMarker)) {
      std::ifstream errorInput(errorMarker, std::ios::binary);
      std::string message{
          std::istreambuf_iterator<char>(errorInput),
          std::istreambuf_iterator<char>()};
      throw std::runtime_error(errorRole + " process failed: " + message);
    }
    if (std::filesystem::exists(marker)) {
      std::ifstream input(marker, std::ios::binary);
      std::string value{
          std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
      if (!value.empty()) {
        return value;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  throw std::runtime_error("The process probe timed out waiting for " + markerName);
}

class AuthoritativeTsoArrivalAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  void timeRegulationEnabled(
      rti1516_2025::LogicalTime const&) override {
    regulationEnabled = true;
  }

  void timeConstrainedEnabled(
      rti1516_2025::LogicalTime const&) override {
    constrainedEnabled = true;
  }

  void discoverObjectInstance(
      rti1516_2025::ObjectInstanceHandle const& instance,
      rti1516_2025::ObjectClassHandle const& objectClass,
      std::wstring const&,
      rti1516_2025::FederateHandle const&) override {
    ++discoveries;
    discoveredInstance = instance;
    discoveredClass = objectClass;
    callbackOrder.push_back('D');
  }

  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::ParameterHandleValueMap const& parameterValues,
      rti1516_2025::VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*,
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::OrderType sentOrder,
      rti1516_2025::OrderType receivedOrder,
      rti1516_2025::MessageRetractionHandle const* retraction) override {
    ++interactions;
    timestampedInteractionClass = interactionClass;
    timestampedProducer = producingFederate;
    timestampedParameterCount = parameterValues.size();
    timestampedSentOrder = sentOrder;
    timestampedReceivedOrder = receivedOrder;
    timestampedRetractionIsValid =
        retraction != nullptr && retraction->isValid();
    if (parameterValues.size() == 1U) {
      auto const& parameterValue = parameterValues.begin()->second;
      auto const* parameterBytes =
          static_cast<unsigned char const*>(parameterValue.data());
      if (parameterBytes != nullptr) {
        timestampedParameterValue.assign(
            parameterBytes, parameterBytes + parameterValue.size());
      }
    }
    auto const* tagBytes =
        static_cast<unsigned char const*>(userSuppliedTag.data());
    if (tagBytes != nullptr) {
      timestampedTag.assign(tagBytes, tagBytes + userSuppliedTag.size());
    }
    callbackOrder.push_back('I');
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      interactionTime = integerTime->getTime();
      interactionTimes.push_back(integerTime->getTime());
    }
  }

  void receiveDirectedInteraction(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      rti1516_2025::ParameterHandleValueMap const& parameterValues,
      rti1516_2025::VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const& producingFederate,
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::OrderType sentOrder,
      rti1516_2025::OrderType receivedOrder,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    ++directedInteractions;
    directedInteractionClass = interactionClass;
    directedObjectInstance = objectInstance;
    directedParameterCount = parameterValues.size();
    directedProducer = producingFederate;
    directedSentOrderIsTimestamp = sentOrder == rti1516_2025::TIMESTAMP;
    directedReceivedOrderIsTimestamp = receivedOrder == rti1516_2025::TIMESTAMP;
    directedRetractionIsValid = optionalRetraction != nullptr &&
        optionalRetraction->isValid();
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      directedInteractionTime = integerTime->getTime();
    }
    auto const* tagBytes =
        static_cast<unsigned char const*>(userSuppliedTag.data());
    if (tagBytes != nullptr) {
      directedTag.assign(tagBytes, tagBytes + userSuppliedTag.size());
    }
    callbackOrder.push_back('I');
  }

  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::ParameterHandleValueMap const& parameterValues,
      rti1516_2025::VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*) override {
    ++receiveOrderInteractions;
    receiveOrderClass = interactionClass;
    receiveOrderProducer = producingFederate;
    if (parameterValues.size() == 1U) {
      auto const& parameterValue = parameterValues.begin()->second;
      auto const* parameterBytes =
          static_cast<unsigned char const*>(parameterValue.data());
      if (parameterBytes != nullptr) {
        receiveOrderParameterValue.assign(
            parameterBytes, parameterBytes + parameterValue.size());
      }
    }
    auto const* bytes = static_cast<unsigned char const*>(userSuppliedTag.data());
    if (bytes != nullptr) {
      receiveOrderTag.assign(bytes, bytes + userSuppliedTag.size());
    }
    callbackOrder.push_back('R');
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    ++grants;
    callbackOrder.push_back('G');
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      grantTime = integerTime->getTime();
    }
  }

  bool regulationEnabled = false;
  bool constrainedEnabled = false;
  std::size_t discoveries = 0U;
  rti1516_2025::ObjectInstanceHandle discoveredInstance;
  rti1516_2025::ObjectClassHandle discoveredClass;
  std::size_t interactions = 0U;
  rti1516_2025::InteractionClassHandle timestampedInteractionClass;
  rti1516_2025::FederateHandle timestampedProducer;
  std::size_t timestampedParameterCount = 0U;
  rti1516_2025::OrderType timestampedSentOrder = rti1516_2025::RECEIVE;
  rti1516_2025::OrderType timestampedReceivedOrder = rti1516_2025::RECEIVE;
  bool timestampedRetractionIsValid = false;
  std::vector<unsigned char> timestampedParameterValue;
  std::vector<unsigned char> timestampedTag;
  std::size_t receiveOrderInteractions = 0U;
  std::size_t grants = 0U;
  std::size_t directedInteractions = 0U;
  rti1516_2025::InteractionClassHandle directedInteractionClass;
  rti1516_2025::ObjectInstanceHandle directedObjectInstance;
  std::size_t directedParameterCount = 0U;
  rti1516_2025::FederateHandle directedProducer;
  bool directedSentOrderIsTimestamp = false;
  bool directedReceivedOrderIsTimestamp = false;
  bool directedRetractionIsValid = false;
  std::vector<unsigned char> directedTag;
  rti1516_2025::InteractionClassHandle receiveOrderClass;
  rti1516_2025::FederateHandle receiveOrderProducer;
  std::vector<unsigned char> receiveOrderParameterValue;
  std::vector<unsigned char> receiveOrderTag;
  std::int64_t interactionTime = -1;
  std::vector<std::int64_t> interactionTimes;
  std::int64_t directedInteractionTime = -1;
  std::int64_t grantTime = -1;
  std::vector<char> callbackOrder;
};

class AuthoritativeTsoAttributeArrivalAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  void timeRegulationEnabled(rti1516_2025::LogicalTime const&) override {
    regulationEnabled = true;
  }

  void timeConstrainedEnabled(rti1516_2025::LogicalTime const&) override {
    constrainedEnabled = true;
  }

  void discoverObjectInstance(
      rti1516_2025::ObjectInstanceHandle const& instance,
      rti1516_2025::ObjectClassHandle const& objectClass,
      std::wstring const&,
      rti1516_2025::FederateHandle const&) override {
    ++discoveries;
    discoveredInstance = instance;
    discoveredClass = objectClass;
    callbackOrder.push_back('D');
  }

  void reflectAttributeValues(
      rti1516_2025::ObjectInstanceHandle const& instance,
      rti1516_2025::AttributeHandleValueMap const& values,
      rti1516_2025::VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*,
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::OrderType sentOrderType,
      rti1516_2025::OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    ++reflections;
    reflectedInstance = instance;
    reflectedProducer = producingFederate;
    reflectedSentOrder = sentOrderType;
    reflectedReceivedOrder = receivedOrderType;
    reflectedRetractionIsValid = optionalRetraction != nullptr &&
        optionalRetraction->isValid();
    if (values.size() == 1U) {
      reflectedAttribute = values.begin()->first;
      auto const& value = values.begin()->second;
      auto const* bytes = static_cast<unsigned char const*>(value.data());
      if (bytes != nullptr) {
        reflectedValue.assign(bytes, bytes + value.size());
      }
    }
    auto const* tagBytes =
        static_cast<unsigned char const*>(userSuppliedTag.data());
    if (tagBytes != nullptr) {
      reflectedTag.assign(tagBytes, tagBytes + userSuppliedTag.size());
    }
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      reflectedTime = integerTime->getTime();
    }
    callbackOrder.push_back('R');
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    ++grants;
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      grantTime = integerTime->getTime();
    }
    callbackOrder.push_back('G');
  }

  bool regulationEnabled = false;
  bool constrainedEnabled = false;
  std::size_t discoveries = 0U;
  std::size_t reflections = 0U;
  std::size_t grants = 0U;
  rti1516_2025::ObjectInstanceHandle discoveredInstance;
  rti1516_2025::ObjectClassHandle discoveredClass;
  rti1516_2025::ObjectInstanceHandle reflectedInstance;
  rti1516_2025::AttributeHandle reflectedAttribute;
  rti1516_2025::FederateHandle reflectedProducer;
  rti1516_2025::OrderType reflectedSentOrder = rti1516_2025::RECEIVE;
  rti1516_2025::OrderType reflectedReceivedOrder = rti1516_2025::RECEIVE;
  bool reflectedRetractionIsValid = false;
  std::vector<unsigned char> reflectedValue;
  std::vector<unsigned char> reflectedTag;
  std::int64_t reflectedTime = -1;
  std::int64_t grantTime = -1;
  std::vector<char> callbackOrder;
};

template <typename Ready>
void evokeUntil(
    rti1516_2025::RTIambassador& ambassador,
    Ready&& ready,
    char const* failureMessage) {
  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::seconds(10);
  while (!ready() && std::chrono::steady_clock::now() < deadline) {
    static_cast<void>(ambassador.evokeCallback(0.01));
  }
  if (!ready()) {
    throw std::runtime_error(failureMessage);
  }
}

rti1516_2025::RtiConfiguration publicProcessConfiguration(
    std::uint16_t port,
    wchar_t const* identity) {
  return rti1516_2025::RtiConfiguration::createConfiguration()
      .withConfigurationName(identity)
      .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
}

int runAuthoritativeTsoService(
    std::filesystem::path const& directory,
    std::string const& role,
    bool pushReceiveOrderEvents = false) {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceOptions serviceOptions;
  serviceOptions.pushReceiveOrderEvents = pushReceiveOrderEvents;
  ProcessFederationServiceHost authority(
      registry,
      composedRestaurantDefinition(),
      {"127.0.0.1", 0U},
      {"process-tso-authoritative-coordinator", 0xB880U},
      std::move(serviceOptions));
  authority.start();
  publishPortMarker(
      directory / (role + ".port"), authority.address().port);
  auto const stopMarker = role + ".stop";
  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::seconds(30);
  while (!std::filesystem::exists(directory / stopMarker) &&
         std::chrono::steady_clock::now() < deadline) {
    if (!authority.running()) {
      authority.rethrowIfFailed();
      throw std::runtime_error("The authoritative process service stopped early.");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  if (!std::filesystem::exists(directory / stopMarker)) {
    throw std::runtime_error("The authoritative process service timed out.");
  }
  authority.stop();
  authority.rethrowIfFailed();
  writeText(directory / (role + ".ok"), "stopped");
  return 0;
}

int runAuthoritativeTsoSender(
    std::filesystem::path const& directory,
    std::string const& role,
    std::string const& receiverRole = "tso-receiver",
    std::int64_t senderAdvanceTime = 5,
    std::int64_t messageTimestamp = 5,
    bool awaitAdvanceGrantBeforeReportingSent = false,
    bool sendEqualTargetMessage = false) {
  auto const port = readPort(directory / "tso-authority.port");
  constexpr wchar_t const* federationName =
      L"process-tso-authoritative-coordinator";
  constexpr wchar_t const* federateName = L"authoritative-tso-sender";
  constexpr wchar_t const* federateType = L"authoritative-tso-type";
  constexpr wchar_t const* interactionName =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr wchar_t const* parameterName = L"TimelinessOk";

  AuthoritativeTsoArrivalAmbassador ambassador;
  rti1516_2025::RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  if (!rti->connect(
           ambassador,
           rti1516_2025::HLA_EVOKED,
           publicProcessConfiguration(port, L"authoritative-tso-sender"))
           .addressUsed) {
    throw std::runtime_error("The sender did not select the authority endpoint.");
  }
  rti->createFederationExecution(
      federationName, L"server-owned-authoritative-tso-fom.xml");
  auto const senderHandle = rti->joinFederationExecution(
      federateName, federateType, federationName);
  if (!senderHandle.isValid()) {
    throw std::runtime_error("The sender Join returned an invalid handle.");
  }
  auto const interactionClass =
      rti->getInteractionClassHandle(interactionName);
  auto const parameter = rti->getParameterHandle(
      interactionClass, parameterName);
  rti->publishInteractionClass(interactionClass);
  rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1));
  evokeUntil(
      *rti,
      [&] { return ambassador.regulationEnabled; },
      "The sender did not receive Time Regulation Enabled.");
  publishPortMarker(directory / (role + ".port"), 1U);

  static_cast<void>(awaitProcessMarker(
      directory,
      receiverRole + ".requested",
      receiverRole));
  std::uint8_t const payload = 0x61U;
  std::uint8_t const tag = 0x58U;
  rti1516_2025::ParameterHandleValueMap values;
  values.emplace(parameter, rti1516_2025::VariableLengthData(&payload, 1U));
  auto const retraction = rti->sendInteraction(
      interactionClass,
      values,
      rti1516_2025::VariableLengthData(&tag, 1U),
      rti1516_2025::HLAinteger64Time(messageTimestamp));
  if (!retraction.isValid()) {
    throw std::runtime_error("The timestamped Send Interaction was rejected.");
  }
  if (sendEqualTargetMessage) {
    auto const equalBoundaryRetraction = rti->sendInteraction(
        interactionClass,
        values,
        rti1516_2025::VariableLengthData(&tag, 1U),
        rti1516_2025::HLAinteger64Time(4));
    if (!equalBoundaryRetraction.isValid()) {
      throw std::runtime_error(
          "The equal-target timestamped Send Interaction was rejected.");
    }
  }
  rti->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(senderAdvanceTime));
  if (awaitAdvanceGrantBeforeReportingSent) {
    evokeUntil(
        *rti,
        [&] { return ambassador.grants == 1U; },
        "The sender did not receive its TAR grant before publishing its send frontier.");
    if (ambassador.grantTime != senderAdvanceTime) {
      throw std::runtime_error(
          "The sender grant did not reach the requested logical time.");
    }
  }
  std::string const sentEvidence = sendEqualTargetMessage
      ? "sent-timestamps-3-and-4"
      : "sent-timestamp-" + std::to_string(messageTimestamp);
  writeText(directory / (role + ".sent"), sentEvidence);

  static_cast<void>(awaitProcessMarker(
      directory,
      receiverRole + ".completed",
      receiverRole));
  writeText(directory / (role + ".completed"), sentEvidence);
  rti->resignFederationExecution(rti1516_2025::NO_ACTION);
  rti->disconnect();
  static_cast<void>(awaitProcessMarker(
      directory,
      role + ".stop",
      role,
      std::chrono::seconds(10)));
  writeText(directory / (role + ".ok"), "stopped");
  return 0;
}

int runAuthoritativeTsoReceiver(
    std::filesystem::path const& directory,
    std::string const& role) {
  auto const port = readPort(directory / "tso-authority.port");
  constexpr wchar_t const* federationName =
      L"process-tso-authoritative-coordinator";
  constexpr wchar_t const* federateName = L"authoritative-tso-receiver";
  constexpr wchar_t const* federateType = L"authoritative-tso-type";
  constexpr wchar_t const* interactionName =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";

  AuthoritativeTsoArrivalAmbassador ambassador;
  rti1516_2025::RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  if (!rti->connect(
           ambassador,
           rti1516_2025::HLA_EVOKED,
           publicProcessConfiguration(port, L"authoritative-tso-receiver"))
           .addressUsed) {
    throw std::runtime_error("The receiver did not select the authority endpoint.");
  }
  auto const receiverHandle = rti->joinFederationExecution(
      federateName, federateType, federationName);
  if (!receiverHandle.isValid()) {
    throw std::runtime_error("The receiver Join returned an invalid handle.");
  }
  auto const interactionClass =
      rti->getInteractionClassHandle(interactionName);
  rti->subscribeInteractionClass(interactionClass, true);
  rti->enableTimeConstrained();
  evokeUntil(
      *rti,
      [&] { return ambassador.constrainedEnabled; },
      "The receiver did not receive Time Constrained Enabled.");
  rti->nextMessageRequestAvailable(rti1516_2025::HLAinteger64Time(10));
  writeText(directory / (role + ".requested"), "requested");
  publishPortMarker(directory / (role + ".port"), 1U);

  static_cast<void>(awaitProcessMarker(
      directory,
      "tso-sender.sent",
      "tso-sender"));
  evokeUntil(
      *rti,
      [&] { return ambassador.grants == 1U; },
      "The receiver did not receive its NMRA grant.");
  if (ambassador.interactions != 1U ||
      ambassador.callbackOrder != std::vector<char>{'I', 'G'} ||
      ambassador.interactionTime != 5 || ambassador.grantTime != 5) {
    throw std::runtime_error(
        "The authority process did not deliver the timestamped interaction before the NMRA grant.");
  }
  writeText(
      directory / (role + ".completed"),
      "interaction-before-grant-at-5");
  rti->resignFederationExecution(rti1516_2025::NO_ACTION);
  rti->disconnect();
  static_cast<void>(awaitProcessMarker(
      directory,
      role + ".stop",
      role,
      std::chrono::seconds(10)));
  writeText(directory / (role + ".ok"), "stopped");
  return 0;
}

int runAuthoritativeTaraTsoReceiver(
    std::filesystem::path const& directory,
    std::string const& role) {
  auto const port = readPort(directory / "tso-authority.port");
  constexpr wchar_t const* federationName =
      L"process-tso-authoritative-coordinator";
  constexpr wchar_t const* federateName = L"authoritative-tara-tso-receiver";
  constexpr wchar_t const* federateType = L"authoritative-tso-type";
  constexpr wchar_t const* interactionName =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";

  AuthoritativeTsoArrivalAmbassador ambassador;
  rti1516_2025::RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  if (!rti->connect(
           ambassador,
           rti1516_2025::HLA_EVOKED,
           publicProcessConfiguration(port, L"authoritative-tara-tso-receiver"))
           .addressUsed) {
    throw std::runtime_error("The TARA receiver did not select the authority endpoint.");
  }
  auto const receiverHandle = rti->joinFederationExecution(
      federateName, federateType, federationName);
  if (!receiverHandle.isValid()) {
    throw std::runtime_error("The TARA receiver Join returned an invalid handle.");
  }
  auto const interactionClass =
      rti->getInteractionClassHandle(interactionName);
  rti->subscribeInteractionClass(interactionClass, true);
  rti->enableTimeConstrained();
  evokeUntil(
      *rti,
      [&] { return ambassador.constrainedEnabled; },
      "The TARA receiver did not receive Time Constrained Enabled.");
  rti->timeAdvanceRequestAvailable(rti1516_2025::HLAinteger64Time(6));
  writeText(directory / (role + ".requested"), "TARA-6-requested");
  publishPortMarker(directory / (role + ".port"), 1U);

  static_cast<void>(awaitProcessMarker(
      directory,
      "tso-tara-sender.sent",
      "tso-tara-sender"));
  evokeUntil(
      *rti,
      [&] { return ambassador.grants == 1U; },
      "The TARA receiver did not receive its grant.");
  if (ambassador.interactions != 1U ||
      ambassador.callbackOrder != std::vector<char>{'I', 'G'} ||
      ambassador.timestampedInteractionClass != interactionClass ||
      ambassador.interactionTime != 5 || ambassador.grantTime != 6 ||
      ambassador.timestampedParameterCount != 1U ||
      ambassador.timestampedParameterValue != std::vector<unsigned char>{0x61U} ||
      ambassador.timestampedTag != std::vector<unsigned char>{0x58U} ||
      ambassador.timestampedSentOrder != rti1516_2025::TIMESTAMP ||
      ambassador.timestampedReceivedOrder != rti1516_2025::TIMESTAMP ||
      !ambassador.timestampedRetractionIsValid) {
    throw std::runtime_error(
        "The authoritative coordinator did not deliver the timestamp-5 interaction before the TARA-6 grant with its public metadata intact.");
  }
  writeText(
      directory / (role + ".completed"),
      "interaction-before-tara-grant-at-6");
  rti->resignFederationExecution(rti1516_2025::NO_ACTION);
  rti->disconnect();
  static_cast<void>(awaitProcessMarker(
      directory,
      role + ".stop",
      role,
      std::chrono::seconds(10)));
  writeText(directory / (role + ".ok"), "stopped");
  return 0;
}

int runAcknowledgedTsoTarReceiver(
    std::filesystem::path const& directory,
    std::string const& role) {
  auto const port = readPort(directory / "tso-authority.port");
  constexpr wchar_t const* federationName =
      L"process-tso-authoritative-coordinator";
  constexpr wchar_t const* federateName =
      L"authoritative-acknowledged-tso-tar-receiver";
  constexpr wchar_t const* federateType = L"authoritative-tso-type";
  constexpr wchar_t const* interactionName =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";

  AuthoritativeTsoArrivalAmbassador ambassador;
  rti1516_2025::RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  if (!rti->connect(
           ambassador,
           rti1516_2025::HLA_EVOKED,
           publicProcessConfiguration(
               port, L"authoritative-acknowledged-tso-tar-receiver"))
           .addressUsed) {
    throw std::runtime_error(
        "The acknowledged-TSO receiver did not select the authority endpoint.");
  }
  auto const receiverHandle = rti->joinFederationExecution(
      federateName, federateType, federationName);
  if (!receiverHandle.isValid()) {
    throw std::runtime_error(
        "The acknowledged-TSO receiver Join returned an invalid handle.");
  }
  auto const interactionClass =
      rti->getInteractionClassHandle(interactionName);
  rti->subscribeInteractionClass(interactionClass, true);
  rti->enableTimeConstrained();
  evokeUntil(
      *rti,
      [&] { return ambassador.constrainedEnabled; },
      "The acknowledged-TSO receiver did not receive Time Constrained Enabled.");

  // Tell the sender it can publish timestamps 3 and 4 and advance to 5. It
  // reports readiness only after TAG(5), making its lookahead-1 frontier 6
  // before this receiver requests TAR(4).
  writeText(directory / (role + ".requested"), "receiver-ready");
  publishPortMarker(directory / (role + ".port"), 1U);
  static_cast<void>(awaitProcessMarker(
      directory,
      "tso-galt-release-sender.sent",
      "tso-galt-release-sender"));
  rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4));
  evokeUntil(
      *rti,
      [&] { return ambassador.grants == 1U; },
      "The receiver did not receive TAR(4) after acknowledging timestamp 3.");
  if (ambassador.interactions != 2U ||
      ambassador.callbackOrder != std::vector<char>{'I', 'I', 'G'} ||
      ambassador.timestampedInteractionClass != interactionClass ||
      ambassador.interactionTimes != std::vector<std::int64_t>{3, 4} ||
      ambassador.interactionTime != 4 || ambassador.grantTime != 4 ||
      ambassador.timestampedParameterCount != 1U ||
      ambassador.timestampedParameterValue != std::vector<unsigned char>{0x61U} ||
      ambassador.timestampedTag != std::vector<unsigned char>{0x58U} ||
      ambassador.timestampedSentOrder != rti1516_2025::TIMESTAMP ||
      ambassador.timestampedReceivedOrder != rti1516_2025::TIMESTAMP ||
      !ambassador.timestampedProducer.isValid() ||
      !ambassador.timestampedRetractionIsValid) {
    throw std::runtime_error(
        "The authoritative coordinator did not deliver timestamp 3 before TAR(4) after regulator TAG(5).");
  }
  writeText(
      directory / (role + ".completed"),
      "timestamps-3-and-4-before-tar-grant-4");
  rti->resignFederationExecution(rti1516_2025::NO_ACTION);
  rti->disconnect();
  static_cast<void>(awaitProcessMarker(
      directory,
      role + ".stop",
      role,
      std::chrono::seconds(10)));
  writeText(directory / (role + ".ok"), "stopped");
  return 0;
}

int runAuthoritativeDirectedTsoSender(
    std::filesystem::path const& directory,
    std::string const& role,
    bool const useNmr,
    bool const useTara = false) {
  std::string const rolePrefix = useTara
      ? "directed-tso-tara"
      : (useNmr ? "directed-tso-nmr" : "directed-tso-nmra");
  auto const port = readPort(directory / (rolePrefix + "-authority.port"));
  constexpr wchar_t const* federationName =
      L"process-directed-tso-authoritative-coordinator";
  constexpr wchar_t const* federateName = L"authoritative-directed-tso-sender";
  constexpr wchar_t const* federateType = L"authoritative-directed-tso-type";
  constexpr wchar_t const* objectClassName =
      L"HLAobjectRoot.Employee.Server";
  constexpr wchar_t const* interactionName =
      L"HLAinteractionRoot.ServerAction.TakeOrder";
  constexpr wchar_t const* privilegeName = L"HLAprivilegeToDeleteObject";

  AuthoritativeTsoArrivalAmbassador ambassador;
  rti1516_2025::RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  if (!rti->connect(
           ambassador,
           rti1516_2025::HLA_EVOKED,
           publicProcessConfiguration(port, L"authoritative-directed-tso-sender"))
           .addressUsed) {
    throw std::runtime_error("The directed sender did not select the authority endpoint.");
  }
  rti->createFederationExecution(
      federationName, L"server-owned-authoritative-tso-fom.xml");
  auto const senderHandle = rti->joinFederationExecution(
      federateName, federateType, federationName);
  if (!senderHandle.isValid()) {
    throw std::runtime_error("The directed sender Join returned an invalid handle.");
  }

  auto const objectClass = rti->getObjectClassHandle(objectClassName);
  auto const privilege = rti->getAttributeHandle(objectClass, privilegeName);
  auto const interactionClass = rti->getInteractionClassHandle(interactionName);
  if (!objectClass.isValid() || !privilege.isValid() ||
      !interactionClass.isValid()) {
    throw std::runtime_error(
        "The directed sender could not resolve the Restaurant FOM target or interaction.");
  }
  rti1516_2025::AttributeHandleSet publishedAttributes;
  publishedAttributes.insert(privilege);
  rti1516_2025::InteractionClassHandleSet directedInteractions;
  directedInteractions.insert(interactionClass);
  rti->publishObjectClassAttributes(objectClass, publishedAttributes);
  rti->publishObjectClassDirectedInteractions(objectClass, directedInteractions);
  rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1));
  evokeUntil(
      *rti,
      [&] { return ambassador.regulationEnabled; },
      "The directed sender did not receive Time Regulation Enabled.");
  publishPortMarker(directory / (role + ".port"), 1U);

  static_cast<void>(awaitProcessMarker(
      directory,
      rolePrefix + "-receiver.ready",
      rolePrefix + "-receiver"));
  auto const target = rti->registerObjectInstance(objectClass);
  if (!target.isValid()) {
    throw std::runtime_error("The directed sender registered an invalid target instance.");
  }
  static_cast<void>(awaitProcessMarker(
      directory,
      rolePrefix + "-receiver.requested",
      rolePrefix + "-receiver"));

  std::uint8_t const tag = 0x3AU;
  rti1516_2025::ParameterHandleValueMap parameterValues;
  auto const retraction = rti->sendDirectedInteraction(
      interactionClass,
      target,
      parameterValues,
      rti1516_2025::VariableLengthData(&tag, 1U),
      rti1516_2025::HLAinteger64Time(5));
  if (!retraction.isValid()) {
    throw std::runtime_error("The timestamped Send Directed Interaction was rejected.");
  }
  rti->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(useTara ? 6 : 5));
  writeText(directory / (role + ".sent"), "sent-directed-timestamp-5");

  static_cast<void>(awaitProcessMarker(
      directory,
      rolePrefix + "-receiver.completed",
      rolePrefix + "-receiver"));
  writeText(directory / (role + ".completed"), "sent-directed-timestamp-5");
  rti->resignFederationExecution(rti1516_2025::NO_ACTION);
  rti->disconnect();
  static_cast<void>(awaitProcessMarker(
      directory,
      role + ".stop",
      role,
      std::chrono::seconds(10)));
  writeText(directory / (role + ".ok"), "stopped");
  return 0;
}

int runAuthoritativeDirectedTsoReceiver(
    std::filesystem::path const& directory,
    std::string const& role,
    bool const useNmr,
    bool const useTara = false) {
  std::string const rolePrefix = useTara
      ? "directed-tso-tara"
      : (useNmr ? "directed-tso-nmr" : "directed-tso-nmra");
  auto const port = readPort(directory / (rolePrefix + "-authority.port"));
  constexpr wchar_t const* federationName =
      L"process-directed-tso-authoritative-coordinator";
  constexpr wchar_t const* federateName = L"authoritative-directed-tso-receiver";
  constexpr wchar_t const* federateType = L"authoritative-directed-tso-type";
  constexpr wchar_t const* objectClassName =
      L"HLAobjectRoot.Employee.Server";
  constexpr wchar_t const* interactionName =
      L"HLAinteractionRoot.ServerAction.TakeOrder";
  constexpr wchar_t const* privilegeName = L"HLAprivilegeToDeleteObject";

  AuthoritativeTsoArrivalAmbassador ambassador;
  rti1516_2025::RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  if (!rti->connect(
           ambassador,
           rti1516_2025::HLA_EVOKED,
           publicProcessConfiguration(port, L"authoritative-directed-tso-receiver"))
           .addressUsed) {
    throw std::runtime_error("The directed receiver did not select the authority endpoint.");
  }
  auto const receiverHandle = rti->joinFederationExecution(
      federateName, federateType, federationName);
  if (!receiverHandle.isValid()) {
    throw std::runtime_error("The directed receiver Join returned an invalid handle.");
  }

  auto const objectClass = rti->getObjectClassHandle(objectClassName);
  auto const privilege = rti->getAttributeHandle(objectClass, privilegeName);
  auto const interactionClass = rti->getInteractionClassHandle(interactionName);
  if (!objectClass.isValid() || !privilege.isValid() ||
      !interactionClass.isValid()) {
    throw std::runtime_error(
        "The directed receiver could not resolve the Restaurant FOM target or interaction.");
  }
  rti1516_2025::AttributeHandleSet subscribedAttributes;
  subscribedAttributes.insert(privilege);
  rti1516_2025::InteractionClassHandleSet directedInteractions;
  directedInteractions.insert(interactionClass);
  rti->subscribeObjectClassAttributes(objectClass, subscribedAttributes);
  rti->subscribeObjectClassDirectedInteractions(
      objectClass, directedInteractions, true);
  rti->enableTimeConstrained();
  evokeUntil(
      *rti,
      [&] { return ambassador.constrainedEnabled; },
      "The directed receiver did not receive Time Constrained Enabled.");
  writeText(directory / (role + ".ready"), "subscribed");
  publishPortMarker(directory / (role + ".port"), 1U);
  evokeUntil(
      *rti,
      [&] { return ambassador.discoveries == 1U; },
      "The directed receiver did not discover its target object.");
  if (useTara) {
    rti->timeAdvanceRequestAvailable(rti1516_2025::HLAinteger64Time(6));
    writeText(directory / (role + ".requested"), "tara-6-pending");
  } else if (useNmr) {
    rti->nextMessageRequest(rti1516_2025::HLAinteger64Time(10));
    writeText(directory / (role + ".requested"), "nmr-10-pending");
  } else {
    rti->nextMessageRequestAvailable(rti1516_2025::HLAinteger64Time(10));
    writeText(directory / (role + ".requested"), "nmra-10-pending");
  }

  static_cast<void>(awaitProcessMarker(
      directory,
      rolePrefix + "-sender.sent",
      rolePrefix + "-sender"));
  evokeUntil(
      *rti,
      [&] { return ambassador.grants == 1U; },
      useTara
          ? "The directed receiver did not receive its TARA grant."
          : (useNmr
                ? "The directed receiver did not receive its NMR grant."
                : "The directed receiver did not receive its NMRA grant."));
  if (ambassador.directedInteractions != 1U ||
      ambassador.callbackOrder != std::vector<char>{'D', 'I', 'G'} ||
      ambassador.discoveries != 1U ||
      ambassador.directedObjectInstance != ambassador.discoveredInstance ||
      ambassador.discoveredClass != objectClass ||
      ambassador.directedInteractionClass != interactionClass ||
      !ambassador.directedProducer.isValid() ||
      ambassador.directedParameterCount != 0U ||
      ambassador.directedTag != std::vector<unsigned char>{0x3AU} ||
      !ambassador.directedSentOrderIsTimestamp ||
      !ambassador.directedReceivedOrderIsTimestamp ||
      !ambassador.directedRetractionIsValid ||
      ambassador.directedInteractionTime != 5 ||
      ambassador.grantTime != (useTara ? 6 : 5)) {
    throw std::runtime_error(
        useTara
            ? "The coordinator did not deliver the timestamp-5 directed callback before grant 6 with its target and metadata intact."
            : "The coordinator did not deliver the timestamped directed callback before grant 5 with its target and metadata intact.");
  }
  writeText(
      directory / (role + ".completed"),
      useTara
          ? "directed-interaction-before-tara-grant-at-6"
          : "directed-interaction-before-grant-at-5");
  rti->resignFederationExecution(rti1516_2025::NO_ACTION);
  rti->disconnect();
  static_cast<void>(awaitProcessMarker(
      directory,
      role + ".stop",
      role,
      std::chrono::seconds(10)));
  writeText(directory / (role + ".ok"), "stopped");
  return 0;
}

int runDisableTcdTsoAuthority(
    std::filesystem::path const& directory,
    std::string const& role) {
  return runAuthoritativeTsoService(directory, role);
}

int runDisableTcdTsoSender(
    std::filesystem::path const& directory,
    std::string const& role) {
  auto const port = readPort(directory / "tcd-authority.port");
  constexpr wchar_t const* federationName =
      L"process-tcd-tso-to-ro-authoritative-coordinator";
  constexpr wchar_t const* federateName = L"process-tcd-tso-sender";
  constexpr wchar_t const* federateType = L"process-tcd-tso-type";
  constexpr wchar_t const* interactionName =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr wchar_t const* parameterName = L"TimelinessOk";

  AuthoritativeTsoArrivalAmbassador ambassador;
  rti1516_2025::RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  if (!rti->connect(
           ambassador,
           rti1516_2025::HLA_EVOKED,
           publicProcessConfiguration(port, L"process-tcd-tso-sender"))
           .addressUsed) {
    throw std::runtime_error("The sender did not select the authority endpoint.");
  }
  rti->createFederationExecution(
      federationName, L"server-owned-process-tcd-tso-fom.xml");
  auto const senderHandle = rti->joinFederationExecution(
      federateName, federateType, federationName);
  if (!senderHandle.isValid()) {
    throw std::runtime_error("The sender Join returned an invalid handle.");
  }
  auto const interactionClass =
      rti->getInteractionClassHandle(interactionName);
  auto const parameter = rti->getParameterHandle(interactionClass, parameterName);
  rti->publishInteractionClass(interactionClass);
  rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1));
  evokeUntil(
      *rti,
      [&] { return ambassador.regulationEnabled; },
      "The sender did not receive Time Regulation Enabled.");
  publishPortMarker(directory / (role + ".port"), 1U);

  static_cast<void>(awaitProcessMarker(
      directory, "tcd-receiver.ready", "tcd-receiver"));
  std::uint8_t const payload = 0x71U;
  std::uint8_t const tag = 0x72U;
  rti1516_2025::ParameterHandleValueMap values;
  values.emplace(parameter, rti1516_2025::VariableLengthData(&payload, 1U));
  auto const retraction = rti->sendInteraction(
      interactionClass,
      values,
      rti1516_2025::VariableLengthData(&tag, 1U),
      rti1516_2025::HLAinteger64Time(5));
  if (!retraction.isValid()) {
    throw std::runtime_error("The timestamped Send Interaction was rejected.");
  }
  writeText(directory / (role + ".sent"), "timestamp-5-queued");
  static_cast<void>(awaitProcessMarker(
      directory, "tcd-receiver.completed", "tcd-receiver"));
  writeText(directory / (role + ".completed"), "queued-tso-delivered-receive-order");
  rti->resignFederationExecution(rti1516_2025::NO_ACTION);
  rti->disconnect();
  static_cast<void>(awaitProcessMarker(
      directory, role + ".stop", role, std::chrono::seconds(10)));
  writeText(directory / (role + ".ok"), "stopped");
  return 0;
}

int runDisableTcdTsoReceiver(
    std::filesystem::path const& directory,
    std::string const& role) {
  auto const port = readPort(directory / "tcd-authority.port");
  constexpr wchar_t const* federationName =
      L"process-tcd-tso-to-ro-authoritative-coordinator";
  constexpr wchar_t const* federateName = L"process-tcd-tso-receiver";
  constexpr wchar_t const* federateType = L"process-tcd-tso-type";
  constexpr wchar_t const* interactionName =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";

  AuthoritativeTsoArrivalAmbassador ambassador;
  rti1516_2025::RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  if (!rti->connect(
           ambassador,
           rti1516_2025::HLA_EVOKED,
           publicProcessConfiguration(port, L"process-tcd-tso-receiver"))
           .addressUsed) {
    throw std::runtime_error("The receiver did not select the authority endpoint.");
  }
  auto const receiverHandle = rti->joinFederationExecution(
      federateName, federateType, federationName);
  if (!receiverHandle.isValid()) {
    throw std::runtime_error("The receiver Join returned an invalid handle.");
  }
  auto const interactionClass =
      rti->getInteractionClassHandle(interactionName);
  rti->subscribeInteractionClass(interactionClass, true);
  rti->enableTimeConstrained();
  evokeUntil(
      *rti,
      [&] { return ambassador.constrainedEnabled; },
      "The receiver did not receive Time Constrained Enabled.");
  publishPortMarker(directory / (role + ".port"), 1U);
  writeText(directory / (role + ".ready"), "time-constrained-enabled");
  static_cast<void>(awaitProcessMarker(
      directory, "tcd-sender.sent", "tcd-sender"));

  // The sender publishes this marker only after Send Interaction returns; the
  // service sends its TSO frame to this receiver before replying to the sender.
  // Query GALT then acts as an ordered stream barrier: its response follows
  // that already-sent frame on the same receiver session, so the process
  // client has buffered and submitted the interaction to the HLA_EVOKED
  // callback queue before this call returns. Do not evoke the callback or
  // assert any particular GALT value/formula here.
  rti1516_2025::HLAinteger64Time observedGalt;
  if (!rti->queryGALT(observedGalt)) {
    throw std::runtime_error(
        "The receiver could not complete the ordered Query GALT queue barrier.");
  }
  if (ambassador.interactions != 0U ||
      ambassador.receiveOrderInteractions != 0U ||
      !ambassador.callbackOrder.empty()) {
    throw std::runtime_error(
        "The receiver invoked a callback before Disable Time Constrained.");
  }

  rti->disableTimeConstrained();
  if (ambassador.interactions != 0U ||
      ambassador.receiveOrderInteractions != 0U ||
      !ambassador.callbackOrder.empty()) {
    throw std::runtime_error(
        "Disable Time Constrained invoked a queued callback eagerly.");
  }
  evokeUntil(
      *rti,
      [&] { return ambassador.receiveOrderInteractions == 1U; },
      "The queued TSO interaction was not delivered as receive order after disabling Time Constrained.");
  if (ambassador.interactions != 0U || ambassador.grants != 0U ||
      ambassador.callbackOrder != std::vector<char>{'R'} ||
      ambassador.receiveOrderClass != interactionClass ||
      !ambassador.receiveOrderProducer.isValid() ||
      ambassador.receiveOrderParameterValue !=
          std::vector<unsigned char>{0x71U} ||
      ambassador.receiveOrderTag != std::vector<unsigned char>{0x72U}) {
    throw std::runtime_error(
        "Disabling Time Constrained did not produce the expected receive-order-only callback.");
  }
  writeText(directory / (role + ".completed"), "queued-tso-delivered-receive-order");
  rti->resignFederationExecution(rti1516_2025::NO_ACTION);
  rti->disconnect();
  static_cast<void>(awaitProcessMarker(
      directory, role + ".stop", role, std::chrono::seconds(10)));
  writeText(directory / (role + ".ok"), "stopped");
  return 0;
}

int runAuthoritativeAttributeTsoAuthority(
    std::filesystem::path const& directory,
    std::string const& role) {
  return runAuthoritativeTsoService(directory, role, true);
}

int runAuthoritativeAttributeTsoSender(
    std::filesystem::path const& directory,
    std::string const& role,
    bool useNmr,
    bool useTara = false) {
  std::string const processPrefix = useTara
      ? "attribute-tso-tara"
      : useNmr ? "attribute-tso-nmr" : "attribute-tso";
  auto const port = readPort(
      directory / (processPrefix + "-authority.port"));
  wchar_t const* const federationName = useTara
      ? L"process-tso-attribute-update-tara-authoritative-coordinator"
      : useNmr
      ? L"process-tso-attribute-update-nmr-authoritative-coordinator"
      : L"process-tso-attribute-update-authoritative-coordinator";
  wchar_t const* const federateName = useTara
      ? L"process-tso-attribute-tara-sender"
      : useNmr ? L"process-tso-attribute-nmr-sender"
               : L"process-tso-attribute-sender";
  wchar_t const* const federateType = useTara
      ? L"process-tso-attribute-tara-type"
      : useNmr ? L"process-tso-attribute-nmr-type"
               : L"process-tso-attribute-type";
  wchar_t const* const configurationName = useTara
      ? L"process-tso-attribute-tara-sender"
      : useNmr ? L"process-tso-attribute-nmr-sender"
               : L"process-tso-attribute-sender";
  constexpr wchar_t const* objectClassName =
      L"HLAobjectRoot.Food.Drink.Soda";
  constexpr wchar_t const* attributeName = L"Flavor";

  AuthoritativeTsoAttributeArrivalAmbassador ambassador;
  rti1516_2025::RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  if (!rti->connect(
           ambassador,
           rti1516_2025::HLA_EVOKED,
           publicProcessConfiguration(port, configurationName))
           .addressUsed) {
    throw std::runtime_error("The attribute sender did not select the coordinator endpoint.");
  }
  rti->createFederationExecution(
      federationName, L"server-owned-process-tso-attribute-update-fom.xml");
  auto const senderHandle = rti->joinFederationExecution(
      federateName, federateType, federationName);
  if (!senderHandle.isValid()) {
    throw std::runtime_error("The attribute sender Join returned an invalid handle.");
  }
  auto const objectClass = rti->getObjectClassHandle(objectClassName);
  auto const attribute = rti->getAttributeHandle(objectClass, attributeName);
  rti->publishObjectClassAttributes(objectClass, {attribute});
  rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1));
  evokeUntil(
      *rti,
      [&] { return ambassador.regulationEnabled; },
      "The attribute sender did not receive Time Regulation Enabled.");
  publishPortMarker(directory / (role + ".port"), 1U);
  writeText(directory / (role + ".ready"), "regulation-enabled");
  static_cast<void>(awaitProcessMarker(
      directory,
      processPrefix + "-receiver.ready",
      processPrefix + "-receiver"));

  auto const objectInstance = rti->registerObjectInstance(objectClass);
  if (!objectInstance.isValid()) {
    throw std::runtime_error("The attribute sender could not register the Soda object.");
  }
  std::array<unsigned char, 1U> valueBytes{0x61U};
  std::array<unsigned char, 1U> tagBytes{0x62U};
  rti1516_2025::AttributeHandleValueMap attributeValues;
  attributeValues.emplace(
      attribute,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()));
  auto const retraction = rti->updateAttributeValues(
      objectInstance,
      attributeValues,
      rti1516_2025::VariableLengthData(tagBytes.data(), tagBytes.size()),
      rti1516_2025::HLAinteger64Time(5));
  if (!retraction.isValid()) {
    throw std::runtime_error("The timestamped Update Attribute Values call was rejected.");
  }
  rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(useTara ? 6 : 5));
  writeText(directory / (role + ".sent"), "sent-timestamp-5");
  static_cast<void>(awaitProcessMarker(
      directory,
      processPrefix + "-receiver.completed",
      processPrefix + "-receiver"));
  writeText(directory / (role + ".completed"), "sent-timestamp-5");
  rti->resignFederationExecution(rti1516_2025::NO_ACTION);
  rti->disconnect();
  static_cast<void>(awaitProcessMarker(
      directory, role + ".stop", role, std::chrono::seconds(10)));
  writeText(directory / (role + ".ok"), "stopped");
  return 0;
}

int runAuthoritativeAttributeTsoReceiver(
    std::filesystem::path const& directory,
    std::string const& role,
    bool useNmr,
    bool useTara = false) {
  std::string const processPrefix = useTara
      ? "attribute-tso-tara"
      : useNmr ? "attribute-tso-nmr" : "attribute-tso";
  auto const port = readPort(
      directory / (processPrefix + "-authority.port"));
  wchar_t const* const federationName = useTara
      ? L"process-tso-attribute-update-tara-authoritative-coordinator"
      : useNmr
      ? L"process-tso-attribute-update-nmr-authoritative-coordinator"
      : L"process-tso-attribute-update-authoritative-coordinator";
  wchar_t const* const federateName = useTara
      ? L"process-tso-attribute-tara-receiver"
      : useNmr ? L"process-tso-attribute-nmr-receiver"
               : L"process-tso-attribute-receiver";
  wchar_t const* const federateType = useTara
      ? L"process-tso-attribute-tara-type"
      : useNmr ? L"process-tso-attribute-nmr-type"
               : L"process-tso-attribute-type";
  wchar_t const* const configurationName = useTara
      ? L"process-tso-attribute-tara-receiver"
      : useNmr ? L"process-tso-attribute-nmr-receiver"
               : L"process-tso-attribute-receiver";
  constexpr wchar_t const* objectClassName =
      L"HLAobjectRoot.Food.Drink.Soda";
  constexpr wchar_t const* attributeName = L"Flavor";

  AuthoritativeTsoAttributeArrivalAmbassador ambassador;
  rti1516_2025::RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  if (!rti->connect(
           ambassador,
           rti1516_2025::HLA_EVOKED,
           publicProcessConfiguration(port, configurationName))
           .addressUsed) {
    throw std::runtime_error("The attribute receiver did not select the coordinator endpoint.");
  }
  static_cast<void>(awaitProcessMarker(
      directory,
      processPrefix + "-sender.ready",
      processPrefix + "-sender"));
  auto const receiverHandle = rti->joinFederationExecution(
      federateName, federateType, federationName);
  if (!receiverHandle.isValid()) {
    throw std::runtime_error("The attribute receiver Join returned an invalid handle.");
  }
  auto const objectClass = rti->getObjectClassHandle(objectClassName);
  auto const attribute = rti->getAttributeHandle(objectClass, attributeName);
  rti->subscribeObjectClassAttributes(objectClass, {attribute}, true);
  rti->enableTimeConstrained();
  evokeUntil(
      *rti,
      [&] { return ambassador.constrainedEnabled; },
      "The attribute receiver did not receive Time Constrained Enabled.");
  if (useTara) {
    rti->timeAdvanceRequestAvailable(rti1516_2025::HLAinteger64Time(6));
  } else if (useNmr) {
    rti->nextMessageRequest(rti1516_2025::HLAinteger64Time(10));
  } else {
    rti->nextMessageRequestAvailable(rti1516_2025::HLAinteger64Time(10));
  }
  publishPortMarker(directory / (role + ".port"), 1U);
  writeText(
      directory / (role + ".ready"),
      useTara ? "tara-6-pending"
              : useNmr ? "nmr-10-pending" : "nmra-10-pending");
  static_cast<void>(awaitProcessMarker(
      directory,
      processPrefix + "-sender.sent",
      processPrefix + "-sender"));
  evokeUntil(
      *rti,
      [&] { return ambassador.grants == 1U; },
      useTara
          ? "The attribute receiver did not receive its TARA grant."
          : useNmr ? "The attribute receiver did not receive its NMR grant."
                   : "The attribute receiver did not receive its NMRA grant.");
  std::int64_t const expectedGrantTime = useTara ? 6 : 5;
  if (ambassador.discoveries != 1U || ambassador.reflections != 1U ||
      ambassador.callbackOrder != std::vector<char>{'D', 'R', 'G'} ||
      ambassador.discoveredClass != objectClass ||
      ambassador.discoveredInstance != ambassador.reflectedInstance ||
      ambassador.reflectedAttribute != attribute ||
      !ambassador.reflectedProducer.isValid() ||
      ambassador.reflectedValue != std::vector<unsigned char>{0x61U} ||
      ambassador.reflectedTag != std::vector<unsigned char>{0x62U} ||
      ambassador.reflectedSentOrder != rti1516_2025::TIMESTAMP ||
      ambassador.reflectedReceivedOrder != rti1516_2025::TIMESTAMP ||
      ambassador.reflectedTime != 5 ||
      ambassador.grantTime != expectedGrantTime ||
      (useTara && !ambassador.reflectedRetractionIsValid)) {
    std::ostringstream diagnostic;
    diagnostic << "The late timestamped attribute update did not precede the "
               << (useTara ? "TARA" : useNmr ? "NMR" : "NMRA")
               << " grant with its metadata intact. "
               << "discoveries=" << ambassador.discoveries
               << " reflections=" << ambassador.reflections
               << " grants=" << ambassador.grants
               << " callback-order=" << std::string(
                      ambassador.callbackOrder.begin(),
                      ambassador.callbackOrder.end())
               << " reflected-time=" << ambassador.reflectedTime
               << " grant-time=" << ambassador.grantTime
               << " retraction-valid="
               << ambassador.reflectedRetractionIsValid
               << " value="
               << (ambassador.reflectedValue.empty()
                       ? -1
                       : static_cast<int>(ambassador.reflectedValue.front()))
               << " tag="
               << (ambassador.reflectedTag.empty()
                       ? -1
                       : static_cast<int>(ambassador.reflectedTag.front()))
               << " sent-order="
               << static_cast<int>(ambassador.reflectedSentOrder)
               << " received-order="
               << static_cast<int>(ambassador.reflectedReceivedOrder)
               << " object-match="
               << (ambassador.discoveredInstance == ambassador.reflectedInstance)
               << " class-match=" << (ambassador.discoveredClass == objectClass)
               << " attribute-match=" << (ambassador.reflectedAttribute == attribute)
               << " producer-valid=" << ambassador.reflectedProducer.isValid();
    throw std::runtime_error(diagnostic.str());
  }
  writeText(
      directory / (role + ".completed"),
      useTara ? "attribute-before-tara-grant-at-6"
              : useNmr ? "attribute-before-nmr-grant-at-5"
                       : "attribute-before-grant-at-5");
  rti->resignFederationExecution(rti1516_2025::NO_ACTION);
  rti->disconnect();
  static_cast<void>(awaitProcessMarker(
      directory, role + ".stop", role, std::chrono::seconds(10)));
  writeText(directory / (role + ".ok"), "stopped");
  return 0;
}

int runServer(std::filesystem::path const& directory) {
  EmbeddedFederationRegistry registry;
  ProcessFomPreparationContext fomContext;
  auto serviceOptions = makeProcessServiceOptions(fomContext);
  ProcessFederationService service(
      registry, composedRestaurantDefinition(), serviceOptions);
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  if (!listener) {
    throw std::runtime_error("The process probe could not open its listener.");
  }
  writeText(directory / "port.txt", std::to_string(listener->address().port));

  std::unique_ptr<ProcessTransportSession> sender;
  std::unique_ptr<ProcessTransportSession> receiver;
  for (int index = 0; index < 2; ++index) {
    auto connection = listener->accept(
        nullptr,
        {"process-probe-server", static_cast<std::uint64_t>(0x71U + index)},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    auto const endpointId = connection->peerIdentity().endpointId;
    if (endpointId == "process-probe-sender" && !sender) {
      sender = std::make_unique<ProcessTransportSession>(std::move(connection));
    } else if (endpointId == "process-probe-receiver" && !receiver) {
      receiver = std::make_unique<ProcessTransportSession>(std::move(connection));
    } else {
      throw std::runtime_error("The process probe received an unexpected endpoint.");
    }
  }
  if (!sender || !receiver) {
    throw std::runtime_error("The process probe did not receive both endpoints.");
  }

  auto senderHandler = service.handlerFor(*sender);
  auto receiverHandler = service.handlerFor(*receiver);
  if (!umbra::test::servePrimaryProcessRequest(*sender, senderHandler) ||
      !umbra::test::servePrimaryProcessRequest(*sender, senderHandler) ||
      !umbra::test::servePrimaryProcessRequest(*receiver, receiverHandler)) {
    throw std::runtime_error("The process probe lost a Create or Join request.");
  }

  auto const senderMember = registry.memberByName(
      L"process-execution", L"process-probe-sender");
  auto const receiverMember = registry.memberByName(
      L"process-execution", L"process-probe-receiver");
  auto const interactionClass = registry.interactionClassHandleFor(
      L"process-execution",
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  if (!senderMember || !receiverMember || !interactionClass) {
    throw std::runtime_error("The process probe registry state is incomplete.");
  }
  if (registry.setInteractionClassPublication(
          L"process-execution", senderMember->id, *interactionClass, true) !=
          InteractionClassDeclarationStatus::applied ||
      registry.setInteractionClassSubscription(
          L"process-execution", receiverMember->id, *interactionClass, true) !=
          InteractionClassDeclarationStatus::applied) {
    throw std::runtime_error("The process probe declarations were rejected.");
  }
  writeText(directory / "interaction-class.txt", std::to_string(*interactionClass));

  if (!umbra::test::servePrimaryProcessRequest(*sender, senderHandler)) {
    throw std::runtime_error("The process probe lost its interaction request.");
  }
  if (!serviceOptions.pushReceiveOrderEvents &&
      !umbra::test::servePrimaryProcessRequest(*receiver, receiverHandler)) {
    throw std::runtime_error("The process probe lost its interaction request.");
  }
  service.detach(*sender);
  service.detach(*receiver);
  sender->connection()->close();
  receiver->connection()->close();
  writeText(directory / "server.ok", "ok\n");
  return 0;
}

// This server role is deliberately kept as a test fixture: the installed
// package smoke owns both federate processes and uses only the official public
// RTIambassador on those client sides.  The fixture supplies the private
// process service that is not itself part of the public IEEE binding yet.
int runPublicServer(
    std::filesystem::path const& directory,
    bool connectionLoss,
    bool parameterized,
    bool objectRegistration,
    bool namedRegistration,
  bool attributeUpdate,
  bool directedRetraction) {
  EmbeddedFederationRegistry registry;
  ProcessFomPreparationContext fomContext;
  auto serviceOptions = makeProcessServiceOptions(fomContext);
  ProcessFederationService service(
      registry, composedRestaurantDefinition(), serviceOptions);
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  if (!listener) {
    throw std::runtime_error(
        "The installable process profile server could not open its listener.");
  }
  writeText(directory / "port.txt", std::to_string(listener->address().port));

  bool const automaticCancelPendingAcquisition = std::filesystem::exists(
      directory / "automatic-cancel-pending-acquisition.mode");
  bool const timestamped =
      std::filesystem::exists(directory / "timestamped.mode");
  bool const automaticDeleteObjects = std::filesystem::exists(
      directory / "automatic-delete-objects.mode");
  bool const automaticDeleteThenDivest = std::filesystem::exists(
      directory / "automatic-delete-then-divest.mode");
  bool const federateLostMomReport = std::filesystem::exists(
      directory / "federate-lost-mom-report.mode");
  std::unique_ptr<ProcessTransportSession> sender;
  std::unique_ptr<ProcessTransportSession> receiver;
  std::unique_ptr<ProcessTransportSession> survivor;
  for (int index = 0; index < 2; ++index) {
    auto connection = listener->accept(
        nullptr,
        {"package-process-server", static_cast<std::uint64_t>(0x91U + index)},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    auto const endpointId = connection->peerIdentity().endpointId;
    if (endpointId == "package-process-sender" && !sender) {
      sender = std::make_unique<ProcessTransportSession>(std::move(connection));
    } else if (endpointId == "package-process-receiver" && !receiver) {
      receiver = std::make_unique<ProcessTransportSession>(std::move(connection));
    } else {
      throw std::runtime_error(
          "The installable process profile server received an unexpected endpoint.");
    }
  }
  if (!sender || !receiver) {
    throw std::runtime_error(
        "The installable process profile server did not receive both clients.");
  }

  auto baseSenderHandler = service.handlerFor(*sender);
  auto senderHandler = [&](TransportServiceMessage const& request) {
    return baseSenderHandler(request);
  };
  auto receiverHandler = service.handlerFor(*receiver);
  std::optional<ProcessTransportServiceDispatcher::Handler> survivorHandler;
  if (!umbra::test::servePrimaryProcessRequest(*sender, senderHandler)) {
    throw std::runtime_error(
        "The installable process profile server lost federate Create.");
  }
  if (!umbra::test::servePrimaryProcessRequest(*sender, senderHandler)) {
    throw std::runtime_error(
        "The installable process profile server lost observer Join.");
  }
  if (!umbra::test::servePrimaryProcessRequest(*receiver, receiverHandler)) {
    throw std::runtime_error(
        "The installable process profile server lost lost-member Join.");
  }
  if (automaticCancelPendingAcquisition) {
    auto connection = listener->accept(
        nullptr,
        {"package-process-server", 0x93U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    if (connection->peerIdentity().endpointId != "package-process-receiver") {
      throw std::runtime_error(
          "The installable process profile pending-acquisition server received an unexpected survivor endpoint.");
    }
    survivor = std::make_unique<ProcessTransportSession>(std::move(connection));
    survivorHandler.emplace(service.handlerFor(*survivor));
    if (!umbra::test::servePrimaryProcessRequest(*survivor, *survivorHandler)) {
      throw std::runtime_error(
          "The installable process profile pending-acquisition server lost survivor Join.");
    }
  }

  auto const senderMember = registry.memberByName(
      L"process-execution", L"package-process-sender");
  auto const receiverMember = registry.memberByName(
      L"process-execution", L"package-process-receiver");
  auto const survivorMember = registry.memberByName(
      L"process-execution", L"package-process-receiver-survivor");
  auto serveExpectedSender = [&](TransportServiceOperation operation) {
    while (true) {
      bool expected = false;
      if (!umbra::test::servePrimaryProcessRequest(
              *sender, senderHandler,
              [&](TransportServiceMessage const& request) {
                if (request.operation ==
                        TransportServiceOperation::report_successful_service_invocation ||
                    request.operation ==
                        TransportServiceOperation::report_successful_void_service_invocation) {
                  return senderHandler(request);
                }
                if (request.operation != operation) {
                  throw std::runtime_error(
                      "The installable process profile server received an unexpected sender operation " +
                      std::to_string(static_cast<unsigned int>(request.operation)) +
                      " (expected " +
                      std::to_string(static_cast<unsigned int>(operation)) + ").");
                }
                expected = true;
                return senderHandler(request);
              })) {
        return false;
      }
      if (expected) {
        return true;
      }
    }
  };
  auto serveExpectedSenderServiceReport = [&] {
    return umbra::test::servePrimaryProcessRequest(
        *sender, senderHandler,
        [&](TransportServiceMessage const& request) {
          if (request.operation !=
                  TransportServiceOperation::report_successful_service_invocation &&
              request.operation !=
                  TransportServiceOperation::report_successful_void_service_invocation) {
            throw std::runtime_error(
                "The installable process profile server expected Send Interaction's service report.");
          }
          return senderHandler(request);
        });
  };
  auto serveExpectedReceiver = [&](TransportServiceOperation operation) {
    return umbra::test::servePrimaryProcessRequest(
        *receiver, receiverHandler,
        [&](TransportServiceMessage const& request) {
          if (request.operation != operation) {
            throw std::runtime_error(
                "The installable directed-retraction server received an unexpected receiver operation " +
                std::to_string(static_cast<unsigned int>(request.operation)) +
                " (expected " +
                std::to_string(static_cast<unsigned int>(operation)) + ").");
          }
          return receiverHandler(request);
        });
  };
  auto serveExpectedReceiverAfterEvokedPolls =
      [&](TransportServiceOperation operation) {
        while (true) {
          bool expected = false;
          if (!umbra::test::servePrimaryProcessRequest(
                  *receiver, receiverHandler,
                  [&](TransportServiceMessage const& request) {
                    if (request.operation ==
                        TransportServiceOperation::receive_interaction) {
                      return receiverHandler(request);
                    }
                    if (request.operation != operation) {
                      throw std::runtime_error(
                          "The installable process profile pending-acquisition server received an unexpected lost-member operation " +
                          std::to_string(static_cast<unsigned int>(request.operation)) +
                          " (expected " +
                          std::to_string(static_cast<unsigned int>(operation)) + ").");
                    }
                    expected = true;
                    return receiverHandler(request);
                  })) {
            return false;
          }
          if (expected) {
            return true;
          }
        }
      };
  auto serveExpectedSenderAfterEvokedPolls =
      [&](TransportServiceOperation operation) {
        while (true) {
          bool expected = false;
          if (!umbra::test::servePrimaryProcessRequest(
                  *sender, senderHandler,
                  [&](TransportServiceMessage const& request) {
                    if (request.operation ==
                        TransportServiceOperation::receive_interaction) {
                      return senderHandler(request);
                    }
                    if (request.operation != operation) {
                      throw std::runtime_error(
                          "The installable process profile server received an unexpected sender operation while draining evoked callbacks.");
                    }
                    expected = true;
                    return senderHandler(request);
                  })) {
            return false;
          }
          if (expected) {
            return true;
          }
        }
      };
  // The package probe runs its public clients against a push-mode service. A
  // normal receive-order callback needs one receiver poll; a pre-delivery
  // timestamped retraction additionally causes the client to acknowledge the
  // suppressed TSO reservation after that poll.
  auto serveReceiverDelivery = [&](bool expectAcknowledgement) {
    if (!umbra::test::servePrimaryProcessRequest(
            *receiver, receiverHandler,
            [&](TransportServiceMessage const& request) {
              if (request.operation == TransportServiceOperation::receive_interaction) {
                return receiverHandler(request);
              }
              if (request.operation ==
                  TransportServiceOperation::acknowledge_tso_delivery) {
                return receiverHandler(request);
              }
              throw std::runtime_error(
                  "The installable directed-retraction server received an unexpected receiver delivery operation.");
            })) {
      return false;
    }
    if (expectAcknowledgement) {
      return umbra::test::servePrimaryProcessRequest(
          *receiver, receiverHandler,
          [&](TransportServiceMessage const& request) {
            if (request.operation !=
                TransportServiceOperation::acknowledge_tso_delivery) {
              throw std::runtime_error(
                  "The installable directed-retraction server lost the receiver TSO acknowledgement.");
            }
            return receiverHandler(request);
          });
    }
    return true;
  };
  auto serveExpectedSurvivor = [&](TransportServiceOperation operation) {
    if (!survivor || !survivorHandler) {
      return false;
    }
    return umbra::test::servePrimaryProcessRequest(
        *survivor, *survivorHandler,
        [&](TransportServiceMessage const& request) {
          if (request.operation != operation) {
            throw std::runtime_error(
                "The installable process profile pending-acquisition server received an unexpected survivor operation.");
          }
          return (*survivorHandler)(request);
        });
  };
  auto serveExpectedSurvivorAfterEvokedPolls =
      [&](TransportServiceOperation operation) {
        if (!survivor || !survivorHandler) {
          return false;
        }
        while (true) {
          bool expected = false;
          if (!umbra::test::servePrimaryProcessRequest(
                  *survivor, *survivorHandler,
                  [&](TransportServiceMessage const& request) {
                    if (request.operation ==
                        TransportServiceOperation::receive_interaction) {
                      return (*survivorHandler)(request);
                    }
                    if (request.operation != operation) {
                      throw std::runtime_error(
                          "The installable process profile pending-acquisition server received an unexpected survivor operation " +
                          std::to_string(static_cast<unsigned int>(request.operation)) +
                          " (expected " +
                          std::to_string(static_cast<unsigned int>(operation)) + ").");
                    }
                    expected = true;
                    return (*survivorHandler)(request);
                  })) {
            return false;
          }
          if (expected) {
            return true;
          }
        }
      };

  auto const interactionClass = registry.interactionClassHandleFor(
      L"process-execution",
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const objectClass = registry.objectClassHandleFor(
      L"process-execution", "HLAobjectRoot.Customer");
  if (!senderMember || !receiverMember || !interactionClass || !objectClass) {
    throw std::runtime_error(
        "The installable process profile registry state is incomplete.");
  }
  if (registry.setInteractionClassPublication(
          L"process-execution", senderMember->id, *interactionClass, true) !=
          InteractionClassDeclarationStatus::applied ||
      registry.setInteractionClassSubscription(
          L"process-execution", receiverMember->id, *interactionClass, true) !=
          InteractionClassDeclarationStatus::applied) {
    throw std::runtime_error(
        "The installable process profile declarations were rejected.");
  }
  writeText(directory / "object-class.txt", std::to_string(*objectClass));
  writeText(directory / "interaction-class.txt", std::to_string(*interactionClass));

  if (automaticCancelPendingAcquisition) {
    if (!survivor || !survivorHandler || !receiverMember || !survivorMember) {
      throw std::runtime_error(
          "The installable process profile pending-acquisition member state is incomplete.");
    }
    auto const privilegeAttribute = registry.attributeHandleFor(
        L"process-execution",
        "HLAobjectRoot.Customer",
        "HLAprivilegeToDeleteObject");
    if (!privilegeAttribute) {
      throw std::runtime_error(
          "The installable process profile pending-acquisition server could not resolve the privilege attribute.");
    }
    if (!serveExpectedSender(TransportServiceOperation::get_object_class_handle) ||
        !serveExpectedReceiver(TransportServiceOperation::get_object_class_handle) ||
        !serveExpectedSurvivor(TransportServiceOperation::get_object_class_handle) ||
        !serveExpectedSender(TransportServiceOperation::get_attribute_handle) ||
        !serveExpectedReceiver(TransportServiceOperation::get_attribute_handle) ||
        !serveExpectedSurvivor(TransportServiceOperation::get_attribute_handle) ||
        !serveExpectedSender(TransportServiceOperation::publish_object_class_attributes) ||
        !serveExpectedReceiver(TransportServiceOperation::publish_object_class_attributes) ||
        !serveExpectedReceiver(TransportServiceOperation::subscribe_object_class_attributes) ||
        !serveExpectedSurvivor(TransportServiceOperation::publish_object_class_attributes) ||
        !serveExpectedSurvivor(TransportServiceOperation::subscribe_object_class_attributes) ||
        !serveExpectedSender(TransportServiceOperation::register_object_instance)) {
      throw std::runtime_error(
          "The installable process profile pending-acquisition server lost a pre-loss operation.");
    }
    // Discovery may be pushed before an evoked client needs to poll. The
    // public ownership queries delimit each recipient's callback drain.
    if (!serveExpectedReceiverAfterEvokedPolls(
            TransportServiceOperation::is_attribute_owned_by_federate) ||
        !serveExpectedSurvivorAfterEvokedPolls(
            TransportServiceOperation::is_attribute_owned_by_federate) ||
        !serveExpectedReceiver(TransportServiceOperation::attribute_ownership_acquisition) ||
        !serveExpectedReceiver(TransportServiceOperation::set_automatic_resign_directive) ||
        !serveExpectedReceiver(TransportServiceOperation::get_automatic_resign_directive)) {
      throw std::runtime_error(
          "The installable process profile pending-acquisition server lost the pending ownership setup.");
    }

    waitForFile(directory / "connection-loss-ready.ok", [](std::filesystem::path const& marker) {
      std::ifstream input(marker, std::ios::binary);
      std::string value;
      input >> value;
      if (!input || value != "ready") {
        throw std::runtime_error(
            "The installable process profile connection-loss readiness marker is invalid.");
      }
      return true;
    });
    auto const lost = registry.connectionLost(
        L"process-execution", receiverMember->id);
    if (lost.status != FederationRegistryStatus::applied) {
      throw std::runtime_error(
          "The installable process profile could not apply pending-acquisition Connection Lost.");
    }
    if (!service.dispatchConnectionLossResult(
            L"process-execution",
            std::move(lost),
            receiverMember->id)) {
      throw std::runtime_error(
          "The installable process profile could not project pending-acquisition Connection Lost callbacks.");
    }
    service.detach(*receiver);
    receiver->connection()->close();
    writeText(directory / "receiver-closed.ok", "ok\n");
    waitForFile(directory / "receiver-loss.ok", [](std::filesystem::path const& marker) {
      std::ifstream input(marker, std::ios::binary);
      std::string value;
      input >> value;
      if (!input || value != "callback-ok") {
        throw std::runtime_error(
            "The installable process profile pending-acquisition loss marker is invalid.");
      }
      return true;
    });

    if (!serveExpectedSenderAfterEvokedPolls(
            TransportServiceOperation::unconditional_attribute_ownership_divestiture) ||
        !serveExpectedSurvivorAfterEvokedPolls(
            TransportServiceOperation::is_attribute_owned_by_federate) ||
        !serveExpectedSurvivor(TransportServiceOperation::resign_federation_execution) ||
        !serveExpectedSender(TransportServiceOperation::resign_federation_execution)) {
      throw std::runtime_error(
          "The installable process profile pending-acquisition server lost post-loss ownership cleanup.");
    }
    service.detach(*survivor);
    service.detach(*sender);
    survivor->connection()->close();
    sender->connection()->close();
    writeText(directory / "server.ok", "ok\n");
    return 0;
  }

  if (directedRetraction) {
    auto const directedObjectClass = registry.objectClassHandleFor(
        L"process-execution", "HLAobjectRoot.Employee.Server");
    auto const directedInteractionClass = registry.interactionClassHandleFor(
        L"process-execution", "HLAinteractionRoot.ServerAction.TakeOrder");
    auto const attribute = registry.attributeHandleFor(
        L"process-execution",
        "HLAobjectRoot.Employee.Server",
        "HLAprivilegeToDeleteObject");
    if (!directedObjectClass || !directedInteractionClass || !attribute) {
      throw std::runtime_error(
          "The installable directed-retraction server could not resolve the directed object or attribute.");
    }
    // The consumer deliberately performs the public declaration and target
    // registration sequence.  Validate that each request crosses this
    // process service boundary before the timestamped send/retract pair.
    if (!serveExpectedSender(TransportServiceOperation::get_object_class_handle) ||
        !serveExpectedSender(TransportServiceOperation::get_interaction_class_handle) ||
        !serveExpectedSender(TransportServiceOperation::get_attribute_handle) ||
        !serveExpectedSender(TransportServiceOperation::publish_object_class_attributes) ||
        !serveExpectedSender(TransportServiceOperation::publish_object_class_directed_interactions) ||
        !serveExpectedSender(TransportServiceOperation::register_object_instance) ||
        !serveExpectedReceiver(TransportServiceOperation::get_object_class_handle) ||
        !serveExpectedReceiver(TransportServiceOperation::get_attribute_handle) ||
        !serveExpectedReceiver(TransportServiceOperation::get_interaction_class_handle) ||
        !serveExpectedReceiver(TransportServiceOperation::subscribe_object_class_attributes) ||
        !serveExpectedReceiver(TransportServiceOperation::subscribe_object_class_directed_interactions)) {
      throw std::runtime_error(
          "The installable directed-retraction server lost a lookup, declaration, or registration request.");
    }
    // The public package consumer drains any queued discovery callback before
    // enabling the sender's time-regulating role.  In push mode that callback
    // may already have been captured while the receiver's lookup/declaration
    // requests were in flight, so the server continues with the sender request
    // sequence here.
    if (!serveExpectedSender(TransportServiceOperation::enable_time_regulation)) {
      throw std::runtime_error(
          "The installable directed-retraction server lost Enable Time Regulation.");
    }
    auto const published = registry.publishedObjectClassAttributeHandles(
        L"process-execution", senderMember->id, *directedObjectClass);
    if (!published || !published->contains(*attribute)) {
      throw std::runtime_error(
          "The installable directed-retraction publication was rejected.");
    }
    char const* directedStage = "sender Send Directed Interaction (positive)";
    if (!serveExpectedSender(TransportServiceOperation::send_directed_interaction)) {
      throw std::runtime_error(
          std::string("The installable directed-retraction server lost ") + directedStage + ".");
    }
    directedStage = "receiver Evoke (positive directed callback)";
    if (!serveReceiverDelivery(false)) {
      throw std::runtime_error(
          std::string("The installable directed-retraction server lost ") + directedStage + ".");
    }
    directedStage = "sender Retract (post-delivery)";
    if (!serveExpectedSender(TransportServiceOperation::retract)) {
      throw std::runtime_error(
          std::string("The installable directed-retraction server lost ") + directedStage + ".");
    }
    directedStage = "receiver Evoke (Request Retraction callback)";
    if (!serveExpectedReceiver(TransportServiceOperation::receive_interaction)) {
      throw std::runtime_error(
          std::string("The installable directed-retraction server lost ") + directedStage + ".");
    }
    directedStage = "sender Send Directed Interaction (pre-delivery retraction)";
    if (!serveExpectedSender(TransportServiceOperation::send_directed_interaction)) {
      throw std::runtime_error(
          std::string("The installable directed-retraction server lost ") + directedStage + ".");
    }
    directedStage = "sender Retract (pre-delivery)";
    if (!serveExpectedSender(TransportServiceOperation::retract)) {
      throw std::runtime_error(
          std::string("The installable directed-retraction server lost ") + directedStage + ".");
    }
    directedStage = "receiver Evoke (suppressed callback)";
    if (!serveReceiverDelivery(true)) {
      throw std::runtime_error(
          std::string("The installable directed-retraction server lost ") + directedStage + ".");
    }
    writeText(directory / "send.ok", "ok\n");
    if (!umbra::test::servePrimaryProcessRequest(*sender, senderHandler) ||
        !umbra::test::servePrimaryProcessRequest(*receiver, receiverHandler)) {
      throw std::runtime_error(
          "The installable directed-retraction server lost a public Resign.");
    }
    service.detach(*sender);
    service.detach(*receiver);
    sender->connection()->close();
    receiver->connection()->close();
    writeText(directory / "server.ok", "ok\n");
    return 0;
  }

  if (namedRegistration) {
    auto const attribute = registry.attributeHandleFor(
        L"process-execution",
        "HLAobjectRoot.Customer",
        "HLAprivilegeToDeleteObject");
    if (!attribute) {
      throw std::runtime_error(
          "The installable process profile named-registration server could not resolve the object attribute.");
    }
    std::size_t reservationCount = 0U;
    std::size_t registrationCount = 0U;
    auto serveNamedSender = [&](TransportServiceOperation operation) {
      return umbra::test::servePrimaryProcessRequest(
          *sender, senderHandler,
          [&](TransportServiceMessage const& request) {
            if (request.operation != operation) {
              throw std::runtime_error(
                  "The installable process profile named-registration server received an unexpected sender operation.");
            }
            auto response = senderHandler(request);
            if (operation == TransportServiceOperation::reserve_object_instance_name &&
                response.status == TransportServiceStatus::ok) {
              auto const result =
                  umbra::detail::decodeProcessFederationReserveObjectInstanceNameResult(
                      response.payload);
              ++reservationCount;
              if (reservationCount == 1U) {
                if (!result.succeeded ||
                    result.status !=
                        umbra::detail::ObjectInstanceNameReservationStatus::applied) {
                  throw std::runtime_error(
                      "The installable named-registration server rejected the reservation.");
                }
              } else if (
                  result.status !=
                  umbra::detail::ObjectInstanceNameReservationStatus::illegal_name) {
                throw std::runtime_error(
                    "The installable named-registration server did not preserve IllegalName.");
              }
            }
            if (operation == TransportServiceOperation::register_object_instance &&
                response.status == TransportServiceStatus::ok) {
              auto const result =
                  umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                      response.payload);
              ++registrationCount;
              if (registrationCount == 1U) {
                if (result.status !=
                        umbra::detail::ObjectInstanceRegistrationStatus::applied ||
                    result.objectInstanceHandle == 0U ||
                    result.objectInstanceName != L"package-process-named-object") {
                  throw std::runtime_error(
                      "The installable named-registration server did not preserve the named registration.");
                }
              } else if (
                  result.status !=
                  umbra::detail::ObjectInstanceRegistrationStatus::object_instance_name_in_use) {
                throw std::runtime_error(
                    "The installable named-registration server did not preserve ObjectInstanceNameInUse.");
              }
            }
            return response;
          });
    };
    if (!serveNamedSender(TransportServiceOperation::get_object_class_handle) ||
        !serveNamedSender(TransportServiceOperation::get_attribute_handle) ||
        !serveNamedSender(TransportServiceOperation::publish_object_class_attributes) ||
        !serveNamedSender(TransportServiceOperation::reserve_object_instance_name) ||
        !serveNamedSender(TransportServiceOperation::register_object_instance) ||
        !serveNamedSender(TransportServiceOperation::register_object_instance) ||
        !serveNamedSender(
            TransportServiceOperation::report_failed_service_invocation) ||
        !serveNamedSender(TransportServiceOperation::reserve_object_instance_name) ||
        !serveNamedSender(
            TransportServiceOperation::report_failed_service_invocation)) {
      throw std::runtime_error(
          "The installable process profile named-registration server lost a public operation.");
    }
    writeText(directory / "send.ok", "ok\n");
    if (!umbra::test::servePrimaryProcessRequest(*sender, senderHandler) ||
        !umbra::test::servePrimaryProcessRequest(*receiver, receiverHandler)) {
      throw std::runtime_error(
          "The installable process profile named-registration server lost a public Resign.");
    }
    service.detach(*sender);
    service.detach(*receiver);
    sender->connection()->close();
    receiver->connection()->close();
    writeText(directory / "server.ok", "ok\n");
    return 0;
  }

  if (objectRegistration) {
    auto const attribute = registry.attributeHandleFor(
        L"process-execution",
        "HLAobjectRoot.Customer",
        "HLAprivilegeToDeleteObject");
    if (!attribute) {
      throw std::runtime_error(
          "The installable process profile could not resolve the object attribute.");
    }
    if (!serveExpectedSender(TransportServiceOperation::get_object_class_handle) ||
        !serveExpectedSender(TransportServiceOperation::get_attribute_handle) ||
        !serveExpectedSender(TransportServiceOperation::publish_object_class_attributes)) {
      throw std::runtime_error(
          "The installable process profile object-registration server lost lookup or publication.");
    }
    auto const published = registry.publishedObjectClassAttributeHandles(
        L"process-execution", senderMember->id, *objectClass);
    if (!published || !published->contains(*attribute)) {
      throw std::runtime_error(
          "The installable process profile object-registration publication was rejected.");
    }
    if (!serveExpectedSender(TransportServiceOperation::register_object_instance)) {
      throw std::runtime_error(
          "The installable process profile object-registration server lost registration.");
    }
    writeText(directory / "send.ok", "ok\n");
    if (!umbra::test::servePrimaryProcessRequest(*sender, senderHandler) ||
        !umbra::test::servePrimaryProcessRequest(*receiver, receiverHandler)) {
      throw std::runtime_error(
          "The installable process profile object-registration server lost a public Resign.");
    }
    service.detach(*sender);
    service.detach(*receiver);
    sender->connection()->close();
    receiver->connection()->close();
    writeText(directory / "server.ok", "ok\n");
    return 0;
  }

  if (attributeUpdate) {
    auto const attribute = registry.attributeHandleFor(
        L"process-execution",
        "HLAobjectRoot.Customer",
        "HLAprivilegeToDeleteObject");
    if (!attribute) {
      throw std::runtime_error(
          "The installable process profile attribute-update server could not resolve the object attribute.");
    }
    auto serveExpectedReceiver = [&](TransportServiceOperation operation) {
      return umbra::test::servePrimaryProcessRequest(
          *receiver, receiverHandler,
          [&](TransportServiceMessage const& request) {
            if (request.operation != operation) {
              throw std::runtime_error(
                  "The installable process profile attribute-update server received an unexpected receiver operation.");
            }
            return receiverHandler(request);
          });
    };
    if (!serveExpectedReceiver(TransportServiceOperation::get_object_class_handle) ||
        !serveExpectedReceiver(TransportServiceOperation::get_attribute_handle) ||
        !serveExpectedReceiver(TransportServiceOperation::subscribe_object_class_attributes) ||
        !serveExpectedSender(TransportServiceOperation::get_object_class_handle) ||
        !serveExpectedSender(TransportServiceOperation::get_attribute_handle) ||
        !serveExpectedSender(TransportServiceOperation::publish_object_class_attributes) ||
        !serveExpectedSender(TransportServiceOperation::register_object_instance) ||
        !serveExpectedSender(TransportServiceOperation::update_attribute_values) ||
        !umbra::test::servePrimaryProcessRequest(*receiver, receiverHandler)) {
      throw std::runtime_error(
          "The installable process profile attribute-update server lost a public operation.");
    }
    writeText(directory / "send.ok", "ok\n");
    if (!umbra::test::servePrimaryProcessRequest(*sender, senderHandler) ||
        !umbra::test::servePrimaryProcessRequest(*receiver, receiverHandler)) {
      throw std::runtime_error(
          "The installable process profile attribute-update server lost a public Resign.");
    }
    service.detach(*sender);
    service.detach(*receiver);
    sender->connection()->close();
    receiver->connection()->close();
    writeText(directory / "server.ok", "ok\n");
    return 0;
  }

  if (automaticDeleteThenDivest) {
    // Each socket has exactly one reader.  Discovery and ownership callbacks
    // can make Evoke issue zero or more receive polls, so fixed cross-socket
    // request ordering would deadlock before the loss-readiness marker.
    std::thread senderReader;
    std::thread receiverReader;
    std::exception_ptr senderError;
    std::exception_ptr receiverError;
    std::atomic_bool lossStarted{false};
    auto serveScript = [&](ProcessTransportSession& session,
                           ProcessTransportServiceDispatcher::Handler const& handler,
                           auto const& expected,
                           std::size_t preLossCount,
                           std::exception_ptr& error) {
      try {
        std::size_t nextOperation = 0U;
        while (nextOperation < expected.size()) {
          if (!umbra::test::servePrimaryProcessRequest(
                  session, handler,
                  [&](TransportServiceMessage const& request) {
                    if (request.operation ==
                            TransportServiceOperation::receive_interaction ||
                        request.operation ==
                            TransportServiceOperation::report_successful_service_invocation ||
                        request.operation ==
                            TransportServiceOperation::report_successful_void_service_invocation) {
                      return handler(request);
                    }
                    if (request.operation != expected[nextOperation] ||
                        (nextOperation >= preLossCount &&
                         !lossStarted.load(std::memory_order_acquire))) {
                      throw std::runtime_error(
                          "The automatic delete-then-divest fixture received an unexpected or premature operation " +
                          std::to_string(static_cast<unsigned int>(request.operation)) +
                          " (expected " +
                          std::to_string(static_cast<unsigned int>(expected[nextOperation])) +
                          ").");
                    }
                    ++nextOperation;
                    return handler(request);
                  })) {
            throw std::runtime_error(
                "The automatic delete-then-divest fixture lost a public request.");
          }
        }
      } catch (...) {
        error = std::current_exception();
      }
    };
    std::array const senderOperations{
        TransportServiceOperation::get_object_class_handle,
        TransportServiceOperation::get_attribute_handle,
        TransportServiceOperation::publish_object_class_attributes,
        TransportServiceOperation::subscribe_object_class_attributes,
        TransportServiceOperation::register_object_instance,
        TransportServiceOperation::get_object_instance_name,
        TransportServiceOperation::unconditional_attribute_ownership_divestiture,
        TransportServiceOperation::get_object_instance_handle,
        TransportServiceOperation::is_attribute_owned_by_federate,
        TransportServiceOperation::resign_federation_execution};
    std::array const receiverOperations{
        TransportServiceOperation::get_object_class_handle,
        TransportServiceOperation::get_attribute_handle,
        TransportServiceOperation::publish_object_class_attributes,
        TransportServiceOperation::subscribe_object_class_attributes,
        TransportServiceOperation::attribute_ownership_acquisition_if_available,
        TransportServiceOperation::is_attribute_owned_by_federate,
        TransportServiceOperation::register_object_instance,
        TransportServiceOperation::set_automatic_resign_directive,
        TransportServiceOperation::get_automatic_resign_directive};
    try {
      senderReader = std::thread([&] {
        serveScript(*sender, senderHandler, senderOperations, 7U, senderError);
      });
      receiverReader = std::thread([&] {
        serveScript(
            *receiver, receiverHandler, receiverOperations,
            receiverOperations.size(), receiverError);
      });
      waitForFile(directory / "connection-loss-ready.ok", [](std::filesystem::path const& marker) {
        std::ifstream input(marker, std::ios::binary);
        std::string value;
        input >> value;
        if (!input || value != "ready") {
          throw std::runtime_error(
              "The automatic delete-then-divest fixture readiness marker is invalid.");
        }
        return true;
      });
      receiverReader.join();
      if (receiverError) {
        std::rethrow_exception(receiverError);
      }
      lossStarted.store(true, std::memory_order_release);
      auto const lost = registry.connectionLost(
          L"process-execution", receiverMember->id);
      if (lost.status != FederationRegistryStatus::applied ||
          !service.dispatchConnectionLossResult(
              L"process-execution", std::move(lost), receiverMember->id)) {
        throw std::runtime_error(
            "The automatic delete-then-divest fixture could not apply Connection Lost.");
      }
      service.detach(*receiver);
      receiver->connection()->close();
      writeText(directory / "receiver-closed.ok", "ok\n");
      waitForFile(directory / "receiver-loss.ok", [](std::filesystem::path const& marker) {
        std::ifstream input(marker, std::ios::binary);
        std::string value;
        input >> value;
        if (!input || value != "callback-ok") {
          throw std::runtime_error(
              "The automatic delete-then-divest fixture loss marker is invalid.");
        }
        return true;
      });
      senderReader.join();
      if (senderError) {
        std::rethrow_exception(senderError);
      }
      service.detach(*sender);
      sender->connection()->close();
      writeText(directory / "server.ok", "ok\n");
      return 0;
    } catch (...) {
      sender->connection()->close();
      receiver->connection()->close();
      if (senderReader.joinable()) {
        senderReader.join();
      }
      if (receiverReader.joinable()) {
        receiverReader.join();
      }
      throw;
    }
  }

  if (connectionLoss) {
    std::thread automaticUnconditionalSender;
    std::exception_ptr automaticUnconditionalSenderError;
    std::atomic_bool automaticUnconditionalLossStarted{false};
    // An exception on the receiver side must not leave a joinable reader or a
    // blocked socket behind.  Normal completion joins the reader explicitly.
    struct SenderReaderCleanup final {
      std::thread& reader;
      std::shared_ptr<ProcessTransportConnection> connection;

      ~SenderReaderCleanup() {
        if (reader.joinable()) {
          connection->close();
          reader.join();
        }
      }
    } automaticUnconditionalCleanup{
        automaticUnconditionalSender, sender->connection()};
    bool const evokedCallbacks = std::filesystem::exists(directory / "evoked.mode");
    bool const automaticUnconditional = std::filesystem::exists(
        directory / "automatic-unconditional.mode");
    if (federateLostMomReport) {
      if (!serveExpectedSender(
              TransportServiceOperation::get_interaction_class_handle)) {
        throw std::runtime_error(
            "The installable process profile federate-lost MOM server lost report-class lookup.");
      }
      for (int index = 0; index < 4; ++index) {
        if (!serveExpectedSender(TransportServiceOperation::get_parameter_handle)) {
          throw std::runtime_error(
              "The installable process profile federate-lost MOM server lost report-parameter lookup.");
        }
      }
      if (!serveExpectedSender(TransportServiceOperation::subscribe_interaction_class)) {
        throw std::runtime_error(
            "The installable process profile federate-lost MOM server lost report subscription.");
      }
    }
    if (automaticUnconditional) {
      if (!serveExpectedReceiver(TransportServiceOperation::get_object_class_handle) ||
          !serveExpectedSender(TransportServiceOperation::get_object_class_handle) ||
          !serveExpectedReceiver(TransportServiceOperation::get_attribute_handle) ||
          !serveExpectedSender(TransportServiceOperation::get_attribute_handle) ||
          !serveExpectedReceiver(TransportServiceOperation::get_attribute_handle) ||
          !serveExpectedSender(TransportServiceOperation::get_attribute_handle) ||
          !serveExpectedReceiver(TransportServiceOperation::publish_object_class_attributes) ||
          !serveExpectedSender(TransportServiceOperation::publish_object_class_attributes) ||
          !serveExpectedSender(TransportServiceOperation::subscribe_object_class_attributes) ||
          !serveExpectedReceiver(TransportServiceOperation::register_object_instance) ||
          !serveExpectedReceiver(TransportServiceOperation::get_object_instance_name)) {
        throw std::runtime_error(
            "The installable process profile automatic-divestiture server lost a pre-loss operation.");
      }
      // Push delivery can finish discovery either with or without an Evoke
      // receive poll.  One sender reader owns this socket from discovery through
      // resignation so an optional poll cannot race with post-loss lookups.
      automaticUnconditionalSender = std::thread([&] {
        try {
          std::array const postLossOperations{
              TransportServiceOperation::get_object_instance_handle,
              TransportServiceOperation::is_attribute_owned_by_federate,
              TransportServiceOperation::is_attribute_owned_by_federate,
              TransportServiceOperation::resign_federation_execution};
          std::size_t nextOperation = 0U;
          while (nextOperation < postLossOperations.size()) {
            if (!umbra::test::servePrimaryProcessRequest(
                    *sender, senderHandler,
                    [&](TransportServiceMessage const& request) {
                      if (request.operation ==
                              TransportServiceOperation::receive_interaction ||
                          request.operation ==
                              TransportServiceOperation::report_successful_service_invocation ||
                          request.operation ==
                              TransportServiceOperation::report_successful_void_service_invocation) {
                        return senderHandler(request);
                      }
                      if (!automaticUnconditionalLossStarted.load(
                              std::memory_order_acquire) ||
                          request.operation != postLossOperations[nextOperation]) {
                        throw std::runtime_error(
                            "The automatic-divestiture fixture received an unexpected survivor operation or a post-loss operation before loss started.");
                      }
                      ++nextOperation;
                      return senderHandler(request);
                    })) {
              throw std::runtime_error(
                  "The automatic-divestiture fixture lost a survivor operation.");
            }
          }
        } catch (...) {
          automaticUnconditionalSenderError = std::current_exception();
        }
      });
      if (!serveExpectedReceiver(TransportServiceOperation::set_automatic_resign_directive) ||
          !serveExpectedReceiver(TransportServiceOperation::get_automatic_resign_directive)) {
        throw std::runtime_error(
            "The automatic-divestiture fixture lost the receiver directive operations.");
      }
    }
    if (automaticDeleteObjects) {
      if (!serveExpectedReceiver(TransportServiceOperation::get_object_class_handle) ||
          !serveExpectedSender(TransportServiceOperation::get_object_class_handle) ||
          !serveExpectedReceiver(TransportServiceOperation::get_attribute_handle) ||
          !serveExpectedSender(TransportServiceOperation::get_attribute_handle) ||
          !serveExpectedReceiver(TransportServiceOperation::publish_object_class_attributes) ||
          !serveExpectedSender(TransportServiceOperation::subscribe_object_class_attributes) ||
          !serveExpectedReceiver(TransportServiceOperation::register_object_instance) ||
          !serveExpectedReceiver(TransportServiceOperation::get_object_instance_name)) {
        throw std::runtime_error(
            "The installable process profile automatic-delete-objects server lost a pre-loss operation.");
      }
      // A pushed discovery may be consumed directly by Evoke without a
      // separate receive poll.  Serve the sender's next request alongside the
      // receiver's directive calls so either valid ordering can make progress.
      std::optional<TransportServiceOperation> firstSenderOperation;
      std::exception_ptr senderError;
      std::thread senderRequest([&] {
        try {
          while (!firstSenderOperation) {
            if (!umbra::test::servePrimaryProcessRequest(
                    *sender, senderHandler,
                    [&](TransportServiceMessage const& request) {
                      if (request.operation ==
                              TransportServiceOperation::report_successful_service_invocation ||
                          request.operation ==
                              TransportServiceOperation::report_successful_void_service_invocation) {
                        return senderHandler(request);
                      }
                      if (request.operation != TransportServiceOperation::receive_interaction &&
                          request.operation !=
                              TransportServiceOperation::get_object_instance_handle) {
                        throw std::runtime_error(
                            "The automatic-delete fixture received an unexpected sender request during discovery.");
                      }
                      firstSenderOperation = request.operation;
                      return senderHandler(request);
                    })) {
              throw std::runtime_error(
                  "The automatic-delete fixture lost the sender discovery request.");
            }
          }
        } catch (...) {
          senderError = std::current_exception();
        }
      });
      try {
        if (!serveExpectedReceiver(TransportServiceOperation::set_automatic_resign_directive) ||
            !serveExpectedReceiver(TransportServiceOperation::get_automatic_resign_directive)) {
          throw std::runtime_error(
              "The automatic-delete fixture lost the receiver directive requests.");
        }
        senderRequest.join();
        if (senderError) {
          std::rethrow_exception(senderError);
        }
        if (firstSenderOperation == TransportServiceOperation::receive_interaction &&
            !serveExpectedSender(TransportServiceOperation::get_object_instance_handle)) {
          throw std::runtime_error(
              "The automatic-delete fixture lost the sender object-name lookup.");
        }
      } catch (...) {
        if (senderRequest.joinable()) {
          sender->connection()->close();
          senderRequest.join();
        }
        throw;
      }
    }
    waitForFile(directory / "connection-loss-ready.ok", [](std::filesystem::path const& marker) {
      std::ifstream input(marker, std::ios::binary);
      std::string value;
      input >> value;
      if (!input || value != "ready") {
        throw std::runtime_error(
            "The installable process profile connection-loss readiness marker is invalid.");
      }
      return true;
    });
    // Model a real process/socket loss on the receiver side.  Apply the
    // registry's automatic-resign policy before closing the transport so the
    // surviving sender cannot route a later interaction to a stale member.
    if (automaticUnconditional) {
      automaticUnconditionalLossStarted.store(true, std::memory_order_release);
    }
    bool const lossApplied = federateLostMomReport
        ? service.connectionLost(
              L"process-execution",
              receiverMember->id,
              L"Process endpoint connection lost")
        : [&] {
            auto const lost = registry.connectionLost(
                L"process-execution", receiverMember->id);
            return lost.status == FederationRegistryStatus::applied &&
                service.dispatchConnectionLossResult(
                    L"process-execution", std::move(lost), receiverMember->id);
          }();
    if (!lossApplied) {
      throw std::runtime_error(
          "The installable process profile could not project receiver Connection Lost callbacks.");
    }
    service.detach(*receiver);
    receiver->connection()->close();
    writeText(directory / "receiver-closed.ok", "ok\n");
    if (federateLostMomReport) {
      if (!serveExpectedSenderAfterEvokedPolls(
              TransportServiceOperation::get_transportation_type_handle) ||
          !serveExpectedSenderAfterEvokedPolls(
              TransportServiceOperation::resign_federation_execution) ||
          !serveExpectedSender(
              TransportServiceOperation::destroy_federation_execution)) {
        throw std::runtime_error(
            "The installable process profile federate-lost MOM server lost a post-report lookup or observer teardown request.");
      }
      waitForFile(directory / "receiver-loss.ok", [](std::filesystem::path const& marker) {
        std::ifstream input(marker, std::ios::binary);
        std::string value;
        input >> value;
        if (!input || value != "callback-ok") {
          throw std::runtime_error(
              "The installable process profile federate-lost MOM callback marker is invalid.");
        }
        return true;
      });
      service.detach(*sender);
      sender->connection()->close();
      writeText(directory / "server.ok", "ok\n");
      return 0;
    }
    waitForFile(directory / "receiver-loss.ok", [](std::filesystem::path const& marker) {
      std::ifstream input(marker, std::ios::binary);
      std::string value;
      input >> value;
      if (!input || value != "callback-ok") {
        throw std::runtime_error(
            "The installable process profile receiver loss marker is invalid.");
      }
      return true;
    });

    if (automaticUnconditional) {
      automaticUnconditionalSender.join();
      if (automaticUnconditionalSenderError) {
        std::rethrow_exception(automaticUnconditionalSenderError);
      }
      service.detach(*sender);
      sender->connection()->close();
      writeText(directory / "server.ok", "ok\n");
      return 0;
    }
    if (automaticDeleteObjects) {
      if (!serveExpectedSenderAfterEvokedPolls(
              TransportServiceOperation::get_object_instance_handle) ||
          !serveExpectedSender(
              TransportServiceOperation::report_failed_service_invocation) ||
          !serveExpectedSender(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The installable process profile automatic-delete-objects server lost a post-loss operation.");
      }
      service.detach(*sender);
      sender->connection()->close();
      writeText(directory / "server.ok", "ok\n");
      return 0;
    }
    // The surviving public sender remains usable after the peer disappears:
    // resolve the class, send a no-recipient interaction, then resign normally.
    if (!serveExpectedSender(TransportServiceOperation::get_interaction_class_handle) ||
        !serveExpectedSender(TransportServiceOperation::send_interaction) ||
        !serveExpectedSenderServiceReport()) {
      throw std::runtime_error(
          "The installable process profile loss server lost lookup, Send Interaction, or its successful-service report.");
    }
    writeText(directory / "send.ok", "ok\n");
    if (!umbra::test::servePrimaryProcessRequest(*sender, senderHandler)) {
      throw std::runtime_error(
          "The installable process profile loss server lost sender Resign.");
    }
    service.detach(*sender);
    sender->connection()->close();
    writeText(directory / "server.ok", "ok\n");
    return 0;
  }

  // Keep every public lookup on the service boundary rather than manufacturing
  // handles in the package consumer. The parameterized mode adds the object
  // class and parameter lookups before the shared Send Interaction request.
  if (!serveExpectedSender(TransportServiceOperation::get_interaction_class_handle)) {
    throw std::runtime_error(
        "The installable process profile server lost the interaction-class lookup.");
  }
  if (parameterized &&
      (!serveExpectedSender(TransportServiceOperation::get_object_class_handle) ||
       !serveExpectedSender(TransportServiceOperation::get_parameter_handle))) {
    throw std::runtime_error(
        "The installable process profile parameterized server lost a lookup.");
  }
  if (!serveExpectedSender(TransportServiceOperation::send_interaction)) {
    throw std::runtime_error(
        "The installable process profile server lost Send Interaction.");
  }
  if (!timestamped && !serveExpectedSenderServiceReport()) {
    throw std::runtime_error(
        "The installable process profile server lost Send Interaction's successful-service report.");
  }
  // The public receiver's Evoke path first sends the legacy receive poll while
  // draining the pushed event frame.  Answer that poll before accepting the
  // lifecycle requests so the client can cross the official callback boundary
  // without leaving an in-flight request on the socket.
  if (!umbra::test::servePrimaryProcessRequest(*receiver, receiverHandler)) {
    throw std::runtime_error(
        "The installable process profile server lost the receiver callback poll.");
  }
  writeText(directory / "send.ok", "ok\n");

  // Both public clients resign through the official RTIambassador before the
  // fixture closes their sockets.  This keeps the package run on the normal
  // lifecycle path instead of relying on destructor cleanup.
  if (!umbra::test::servePrimaryProcessRequest(*sender, senderHandler) ||
      !umbra::test::servePrimaryProcessRequest(*receiver, receiverHandler)) {
    throw std::runtime_error(
        "The installable process profile server lost a public Resign.");
  }
  service.detach(*sender);
  service.detach(*receiver);
  sender->connection()->close();
  receiver->connection()->close();
  writeText(directory / "server.ok", "ok\n");
  return 0;
}

// Public ownership service-report TCK cases have two or three independent
// ambassadors depending on whether a separate MOM observer is required. Serve
// each process session on its own worker so lookups, service calls, and callback
// drains can interleave as they do for an external coordinator.
int runPublicOwnershipServiceReportServer(
    std::filesystem::path const& directory,
    std::size_t clientCount,
    std::string const& fixtureName) {
  if (clientCount < 2U || clientCount > 3U) {
    throw std::runtime_error("Unsupported ownership service-report client count.");
  }
  EmbeddedFederationRegistry registry;
  ProcessFomPreparationContext fomContext;
  auto const serviceOptions = makeProcessServiceOptions(fomContext);
  ProcessFederationService service(
      registry, composedRestaurantDefinition(), serviceOptions);
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  if (!listener) {
    throw std::runtime_error(
        "The installable Confirm Divestiture server could not open its listener.");
  }
  writeText(directory / "port.txt", std::to_string(listener->address().port));

  std::vector<std::unique_ptr<ProcessTransportSession>> sessions(clientCount);
  for (std::size_t index = 0U; index < sessions.size(); ++index) {
    auto connection = listener->accept(
        nullptr,
        {"package-process-ownership-service-report-server",
         static_cast<std::uint64_t>(0xA1U + index)},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    if (!connection) {
      throw std::runtime_error(
          "The installable " + fixtureName + " server lost a client connection.");
    }
    auto const expectedEndpoint = index == 0U
        ? "package-process-sender"
        : "package-process-receiver";
    if (connection->peerIdentity().endpointId != expectedEndpoint) {
      throw std::runtime_error(
          "The installable " + fixtureName + " server received endpoint '" +
          connection->peerIdentity().endpointId + "' for client " +
          std::to_string(index) + ".");
    }
    sessions[index] = std::make_unique<ProcessTransportSession>(
        std::move(connection));
  }

  std::vector<std::exception_ptr> failures(sessions.size());
  std::vector<std::thread> workers(sessions.size());
  for (std::size_t index = 0U; index < sessions.size(); ++index) {
    auto handler = service.handlerFor(*sessions[index]);
    workers[index] = std::thread(
        [&, index, handler = std::move(handler)]() mutable {
          try {
            while (umbra::test::servePrimaryProcessRequest(
                *sessions[index], handler)) {
            }
          } catch (...) {
            failures[index] = std::current_exception();
          }
          service.detach(*sessions[index]);
          sessions[index]->connection()->close();
        });
  }
  for (auto& worker : workers) {
    worker.join();
  }
  for (auto const& failure : failures) {
    if (failure) {
      std::rethrow_exception(failure);
    }
  }
  writeText(directory / "server.ok", "ok\n");
  return 0;
}

// The save/restore package lane deliberately uses one installed public
// RTIambassador and one process endpoint.  Keeping this choreography separate
// from the two-federate interaction lanes makes its callback ordering and
// service-operation contract directly queryable without introducing a second
// client whose lifecycle could mask a save/restore defect.
int runPublicSaveRestoreServer(
    std::filesystem::path const& directory,
    bool restoreFailure,
    bool restoreAbort,
    bool restoreStatus) {
  EmbeddedFederationRegistry registry;
  ProcessFomPreparationContext fomContext;
  auto serviceOptions = makeProcessServiceOptions(fomContext);
  // Keep the save/restore choreography on the deterministic pull boundary;
  // each callback drain below must correspond to an explicit
  // receive_interaction request, matching the native process lifecycle case.
  serviceOptions.pushReceiveOrderEvents = false;
  ProcessFederationService service(
      registry, composedRestaurantDefinition(), serviceOptions);
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  if (!listener) {
    throw std::runtime_error(
        "The installable save/restore profile server could not open its listener.");
  }
  writeText(directory / "port.txt", std::to_string(listener->address().port));

  auto connection = listener->accept(
      nullptr,
      {"package-process-save-restore-server", 0x94U},
      [](std::wstring) {},
      [](std::wstring) { return false; });
  if (!connection || connection->peerIdentity().endpointId !=
                         "package-process-sender") {
    throw std::runtime_error(
        "The installable save/restore profile server received an unexpected endpoint.");
  }
  ProcessTransportSession session(connection);
  auto handler = service.handlerFor(session);
  auto serveExpected = [&](TransportServiceOperation operation,
                           char const* description) {
    if (!umbra::test::servePrimaryProcessRequest(
            session, handler,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    std::string(description) +
                    " received an unexpected process operation.");
              }
              return handler(request);
            })) {
      throw std::runtime_error(description);
    }
  };

  serveExpected(
      TransportServiceOperation::create_federation_execution,
      "The installable save/restore profile server lost Create.");
  serveExpected(
      TransportServiceOperation::join_federation_execution,
      "The installable save/restore profile server lost Join.");
  serveExpected(
      TransportServiceOperation::request_federation_save,
      "The installable save/restore profile server lost Request Federation Save.");
  serveExpected(
      TransportServiceOperation::receive_interaction,
      "The installable save/restore profile server lost save-initiation polling.");
  serveExpected(
      TransportServiceOperation::federate_save_begun,
      "The installable save/restore profile server lost Federate Save Begun.");
  serveExpected(
      TransportServiceOperation::federate_save_complete,
      "The installable save/restore profile server lost Federate Save Complete.");
  serveExpected(
      TransportServiceOperation::receive_interaction,
      "The installable save/restore profile server lost save-completion polling.");
  serveExpected(
      TransportServiceOperation::request_federation_restore,
      "The installable save/restore profile server lost Request Federation Restore.");
  serveExpected(
      TransportServiceOperation::receive_interaction,
      "The installable save/restore profile server lost restore-request polling.");
  serveExpected(
      TransportServiceOperation::receive_interaction,
      "The installable save/restore profile server lost restore-begin polling.");
  serveExpected(
      TransportServiceOperation::receive_interaction,
      "The installable save/restore profile server lost restore-initiation polling.");
  if (restoreStatus) {
    serveExpected(
        TransportServiceOperation::query_federation_restore_status,
        "The installable save/restore-status profile server lost Query Federation Restore Status.");
    serveExpected(
        TransportServiceOperation::receive_interaction,
        "The installable save/restore-status profile server lost restore-status polling.");
    serveExpected(
        TransportServiceOperation::federate_restore_complete,
        "The installable save/restore-status profile server lost Federate Restore Complete.");
    serveExpected(
        TransportServiceOperation::receive_interaction,
        "The installable save/restore-status profile server lost restore-completion polling.");
  } else {
    serveExpected(
        restoreAbort
            ? TransportServiceOperation::abort_federation_restore
            : (restoreFailure
                   ? TransportServiceOperation::federate_restore_not_complete
                   : TransportServiceOperation::federate_restore_complete),
        restoreAbort
            ? "The installable save/restore-abort profile server lost Abort Federation Restore."
            : (restoreFailure
                   ? "The installable save/restore-failure profile server lost Federate Restore Not Complete."
                   : "The installable save/restore profile server lost Federate Restore Complete."));
    serveExpected(
        TransportServiceOperation::receive_interaction,
        restoreAbort
            ? "The installable save/restore-abort profile server lost restore-abort polling."
            : (restoreFailure
                   ? "The installable save/restore-failure profile server lost restore-failure polling."
                   : "The installable save/restore profile server lost restore-completion polling."));
  }
  serveExpected(
      TransportServiceOperation::resign_federation_execution,
      "The installable save/restore profile server lost Resign.");

  service.detach(session);
  session.connection()->close();
  writeText(directory / "server.ok", "ok\n");
  return 0;
}

int runSender(std::filesystem::path const& directory) {
  auto const port = readPort(directory / "port.txt");
  ProcessFederationClient client(
      {"127.0.0.1", port}, {"process-probe-sender", 0x81U});
  client.createFederationExecution(L"process-execution");
  auto const joinResult = client.joinFederationExecution(
      L"process-execution",
      L"process-probe-sender",
      L"process-probe-sender");
  writeText(directory / "sender-id.txt", std::to_string(joinResult.federateId));
  auto const interactionClass = readHandle(directory / "interaction-class.txt");
  std::vector<std::uint8_t> const payload{0x50U, 0x52U, 0x4fU, 0x42U};
  auto const sendResult = client.sendInteraction(
      L"process-execution",
      joinResult.federateId,
      interactionClass,
      {},
      payload);
  if (sendResult.recipientCount != 1U) {
    throw std::runtime_error("The process probe Send Interaction had the wrong fanout.");
  }
  client.close();
  writeText(directory / "sender.ok", "ok\n");
  return 0;
}

int runReceiver(std::filesystem::path const& directory) {
  auto const port = readPort(directory / "port.txt");
  ProcessFederationClient client(
      {"127.0.0.1", port}, {"process-probe-receiver", 0x82U});
  auto const joinResult = client.joinFederationExecution(
      L"process-execution",
      L"process-probe-receiver",
      L"process-probe-receiver");
  if (joinResult.federateId == 0U) {
    throw std::runtime_error("The process probe receiver Join returned no identity.");
  }
  auto const interactionClass = readHandle(directory / "interaction-class.txt");
  auto const senderId = readHandle(directory / "sender-id.txt");
  std::vector<std::uint8_t> const expectedPayload{0x50U, 0x52U, 0x4fU, 0x42U};
  ProbeFederateAmbassador ambassador;
  // The server emits a pushed event after Send Interaction.  Its arrival and
  // the client's background dispatch are concurrent with this receiver, so
  // wait at the evoked callback boundary rather than sampling either queue.
  client.attachCallbackBridge(ambassador);
  static_cast<void>(client.evokeOne(std::chrono::seconds(3)));
  if (!ambassador.received) {
    throw std::runtime_error("The process probe callback was not delivered.");
  }
  auto const transportationValue =
      rti1516_2025::umbra_binding_detail::standardTransportationTypeValue(
          L"HLAreliable");
  if (!transportationValue || !ambassador.received ||
      ambassador.interactionClassHandle !=
          rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
              interactionClass) ||
      ambassador.transportationTypeHandle !=
          rti1516_2025::umbra_binding_detail::makeTransportationTypeHandle(
              *transportationValue) ||
      ambassador.producingFederateHandle !=
          rti1516_2025::umbra_binding_detail::makeFederateHandle(
              senderId) ||
      ambassador.parameterCount != 0U ||
      ambassador.tag != expectedPayload || ambassador.hasOptionalSentRegions) {
    throw std::runtime_error("The process probe official callback was not preserved.");
  }
  client.close();
  writeText(directory / "receiver.ok", "callback-ok\n");
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    return 2;
  }
  auto const directory = std::filesystem::path(argv[2]);
  try {
    if (std::string(argv[1]) == "server") {
      return runServer(directory);
    }
    if (std::string(argv[1]) == "public-server") {
      return runPublicServer(directory, false, false, false, false, false, false);
    }
    if (std::string(argv[1]) == "public-server-loss") {
      return runPublicServer(directory, true, false, false, false, false, false);
    }
    if (std::string(argv[1]) == "public-server-parameterized") {
      return runPublicServer(directory, false, true, false, false, false, false);
    }
    if (std::string(argv[1]) == "public-server-object-registration") {
      return runPublicServer(directory, false, false, true, false, false, false);
    }
    if (std::string(argv[1]) == "public-server-named-registration") {
      return runPublicServer(directory, false, false, false, true, false, false);
    }
    if (std::string(argv[1]) == "public-server-attribute-update") {
      return runPublicServer(directory, false, false, false, false, true, false);
    }
    if (std::string(argv[1]) == "public-server-directed-retraction") {
      return runPublicServer(directory, false, false, false, false, false, true);
    }
    if (std::string(argv[1]) == "public-server-confirm-divestiture") {
      return runPublicOwnershipServiceReportServer(
          directory, 3U, "Confirm Divestiture");
    }
    if (std::string(argv[1]) == "public-server-query-ownership") {
      return runPublicOwnershipServiceReportServer(
          directory, 2U, "Query Attribute Ownership");
    }
    if (std::string(argv[1]) == "public-server-cancel-ownership-acquisition") {
      return runPublicOwnershipServiceReportServer(
          directory, 3U, "Cancel Attribute Ownership Acquisition");
    }
    if (std::string(argv[1]) == "public-server-save-restore") {
      return runPublicSaveRestoreServer(directory, false, false, false);
    }
    if (std::string(argv[1]) == "public-server-save-restore-failure") {
      return runPublicSaveRestoreServer(directory, true, false, false);
    }
    if (std::string(argv[1]) == "public-server-save-restore-abort") {
      return runPublicSaveRestoreServer(directory, false, true, false);
    }
    if (std::string(argv[1]) == "public-server-save-restore-status") {
      return runPublicSaveRestoreServer(directory, false, false, true);
    }
    if (std::string(argv[1]) == "sender") {
      return runSender(directory);
    }
    if (std::string(argv[1]) == "receiver") {
      return runReceiver(directory);
    }
    if (std::string(argv[1]) == "tso-authority") {
      return runAuthoritativeTsoService(directory, argv[1]);
    }
    if (std::string(argv[1]) == "tso-sender") {
      return runAuthoritativeTsoSender(directory, argv[1]);
    }
    if (std::string(argv[1]) == "tso-receiver") {
      return runAuthoritativeTsoReceiver(directory, argv[1]);
    }
    if (std::string(argv[1]) == "tso-tara-sender") {
      return runAuthoritativeTsoSender(
          directory, argv[1], "tso-tara-receiver", 6);
    }
    if (std::string(argv[1]) == "tso-tara-receiver") {
      return runAuthoritativeTaraTsoReceiver(directory, argv[1]);
    }
    if (std::string(argv[1]) == "tso-galt-release-sender") {
      return runAuthoritativeTsoSender(
          directory,
          argv[1],
          "tso-galt-release-receiver",
          5,
          3,
          true,
          true);
    }
    if (std::string(argv[1]) == "tso-galt-release-receiver") {
      return runAcknowledgedTsoTarReceiver(directory, argv[1]);
    }
    if (std::string(argv[1]) == "no-tso-coordinator-authority") {
      return runAuthoritativeTsoService(directory, argv[1]);
    }
    if (std::string(argv[1]) == "no-tso-coordinator-regulator" ||
        std::string(argv[1]) == "no-tso-coordinator-requester") {
      return umbra::test::runNoTsoCoordinatorClientProbe(directory, argv[1]);
    }
    if (std::string(argv[1]) == "delete-tso-nmr-authority" ||
        std::string(argv[1]) == "delete-tso-nmra-authority" ||
        std::string(argv[1]) == "delete-tso-tara-authority") {
      return runAuthoritativeTsoService(directory, argv[1]);
    }
    if (std::string(argv[1]) == "delete-tso-nmr-sender" ||
        std::string(argv[1]) == "delete-tso-nmr-receiver" ||
        std::string(argv[1]) == "delete-tso-nmra-sender" ||
        std::string(argv[1]) == "delete-tso-nmra-receiver" ||
        std::string(argv[1]) == "delete-tso-tara-sender" ||
        std::string(argv[1]) == "delete-tso-tara-receiver") {
      return umbra::test::runDeleteTsoCoordinatorClientProbe(directory, argv[1]);
    }
    if (std::string(argv[1]) == "delete-tso-nmr-sender" ||
        std::string(argv[1]) == "delete-tso-nmr-receiver" ||
        std::string(argv[1]) == "delete-tso-nmra-sender" ||
        std::string(argv[1]) == "delete-tso-nmra-receiver") {
      return umbra::test::runDeleteTsoCoordinatorClientProbe(directory, argv[1]);
    }
    if (std::string(argv[1]) == "directed-tso-nmra-authority" ||
        std::string(argv[1]) == "directed-tso-nmr-authority" ||
        std::string(argv[1]) == "directed-tso-tara-authority") {
      return runAuthoritativeTsoService(directory, argv[1]);
    }
    if (std::string(argv[1]) == "directed-tso-tara-sender") {
      return runAuthoritativeDirectedTsoSender(directory, argv[1], false, true);
    }
    if (std::string(argv[1]) == "directed-tso-tara-receiver") {
      return runAuthoritativeDirectedTsoReceiver(directory, argv[1], false, true);
    }
    if (std::string(argv[1]) == "directed-tso-nmra-sender") {
      return runAuthoritativeDirectedTsoSender(directory, argv[1], false);
    }
    if (std::string(argv[1]) == "directed-tso-nmra-receiver") {
      return runAuthoritativeDirectedTsoReceiver(directory, argv[1], false);
    }
    if (std::string(argv[1]) == "directed-tso-nmr-sender") {
      return runAuthoritativeDirectedTsoSender(directory, argv[1], true);
    }
    if (std::string(argv[1]) == "directed-tso-nmr-receiver") {
      return runAuthoritativeDirectedTsoReceiver(directory, argv[1], true);
    }
    if (std::string(argv[1]) == "tcd-authority") {
      return runDisableTcdTsoAuthority(directory, argv[1]);
    }
    if (std::string(argv[1]) == "tcd-sender") {
      return runDisableTcdTsoSender(directory, argv[1]);
    }
    if (std::string(argv[1]) == "tcd-receiver") {
      return runDisableTcdTsoReceiver(directory, argv[1]);
    }
    if (std::string(argv[1]) == "attribute-tso-authority") {
      return runAuthoritativeAttributeTsoAuthority(directory, argv[1]);
    }
    if (std::string(argv[1]) == "attribute-tso-sender") {
      return runAuthoritativeAttributeTsoSender(directory, argv[1], false);
    }
    if (std::string(argv[1]) == "attribute-tso-receiver") {
      return runAuthoritativeAttributeTsoReceiver(directory, argv[1], false);
    }
    if (std::string(argv[1]) == "attribute-tso-nmr-authority") {
      return runAuthoritativeAttributeTsoAuthority(directory, argv[1]);
    }
    if (std::string(argv[1]) == "attribute-tso-nmr-sender") {
      return runAuthoritativeAttributeTsoSender(directory, argv[1], true);
    }
    if (std::string(argv[1]) == "attribute-tso-nmr-receiver") {
      return runAuthoritativeAttributeTsoReceiver(directory, argv[1], true);
    }
    if (std::string(argv[1]) == "attribute-tso-tara-authority") {
      return runAuthoritativeAttributeTsoAuthority(directory, argv[1]);
    }
    if (std::string(argv[1]) == "attribute-tso-tara-sender") {
      return runAuthoritativeAttributeTsoSender(
          directory, argv[1], false, true);
    }
    if (std::string(argv[1]) == "attribute-tso-tara-receiver") {
      return runAuthoritativeAttributeTsoReceiver(
          directory, argv[1], false, true);
    }
  } catch (std::exception const& error) {
    try {
      writeText(directory / (std::string(argv[1]) + ".error"), error.what());
    } catch (...) {
    }
    return 1;
  } catch (rti1516_2025::Exception const& error) {
    try {
      auto narrow = [](std::wstring const& value) {
        std::string result;
        result.reserve(value.size());
        for (wchar_t character : value) {
          result.push_back(character >= 0 && character <= 0x7f
                  ? static_cast<char>(character)
                  : '?');
        }
        return result;
      };
      writeText(
          directory / (std::string(argv[1]) + ".error"),
          narrow(error.name()) + ": " + narrow(error.what()));
    } catch (...) {
    }
    return 1;
  }
  return 2;
}
