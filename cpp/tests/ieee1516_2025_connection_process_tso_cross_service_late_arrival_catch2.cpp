#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service_host.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/fom_validation.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"

namespace {

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RtiConfiguration;
using rti1516_2025::VariableLengthData;
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FomCompositionStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceHost;
using umbra::detail::ProcessTransportAddress;

std::filesystem::path resourcePath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relative;
}

PrevalidatedFomModule validateModule(
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
  if (result.status != umbra::detail::FomValidationStatus::valid ||
      !result.module) {
    throw std::runtime_error("The cross-service TSO FOM module did not validate.");
  }
  return *result.module;
}

FederationDefinition composedDefinition() {
  std::vector<PrevalidatedFomModule> modules{
      validateModule(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:cross-service-tso-mim"),
      validateModule(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:cross-service-tso-restaurant"),
  };
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid || !result.catalog ||
      !result.fdd) {
    throw std::runtime_error("The cross-service TSO FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

RtiConfiguration configurationFor(
    ProcessTransportAddress const& address,
    wchar_t const* name) {
  return RtiConfiguration::createConfiguration()
      .withConfigurationName(name)
      .withRtiAddress(
          L"tcp://127.0.0.1:" + std::to_wstring(address.port));
}

class ArrivalAmbassador final : public NullFederateAmbassador {
 public:
  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const&,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*,
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::OrderType,
      rti1516_2025::OrderType,
      rti1516_2025::MessageRetractionHandle const*) override {
    ++interactions;
    callbackOrder.push_back('I');
    producer = producingFederate;
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      interactionTime = integerTime->getTime();
    }
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    ++grants;
    callbackOrder.push_back('G');
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      grantTime = integerTime->getTime();
    }
  }

  std::size_t interactions = 0U;
  std::size_t grants = 0U;
  std::int64_t interactionTime = -1;
  std::int64_t grantTime = -1;
  rti1516_2025::FederateHandle producer;
  std::vector<char> callbackOrder;
};

class AttributeArrivalAmbassador final : public NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      rti1516_2025::ObjectClassHandle const&,
      std::wstring const&,
      rti1516_2025::FederateHandle const&) override {
    discoveredObject = objectInstance;
    discovered = true;
    callbackOrder.push_back('D');
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
      rti1516_2025::MessageRetractionHandle const*) override {
    ++reflections;
    callbackOrder.push_back('A');
    reflectedObject = objectInstance;
    producer = producingFederate;
    sentOrderType = sentOrder;
    receivedOrderType = receivedOrder;
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      reflectionTime = integerTime->getTime();
    }
    if (!attributeValues.empty() &&
        attributeValues.begin()->second.size() != 0U) {
      auto const& bytes = attributeValues.begin()->second;
      auto const* data = static_cast<std::uint8_t const*>(bytes.data());
      reflectedValue.assign(data, data + bytes.size());
    }
    if (userSuppliedTag.size() != 0U) {
      auto const* data =
          static_cast<std::uint8_t const*>(userSuppliedTag.data());
      reflectedTag.assign(data, data + userSuppliedTag.size());
    }
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    ++grants;
    callbackOrder.push_back('G');
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      grantTime = integerTime->getTime();
    }
  }

  bool discovered = false;
  std::size_t reflections = 0U;
  std::size_t grants = 0U;
  std::int64_t reflectionTime = -1;
  std::int64_t grantTime = -1;
  rti1516_2025::ObjectInstanceHandle discoveredObject;
  rti1516_2025::ObjectInstanceHandle reflectedObject;
  rti1516_2025::FederateHandle producer;
  rti1516_2025::OrderType sentOrderType = rti1516_2025::RECEIVE;
  rti1516_2025::OrderType receivedOrderType = rti1516_2025::RECEIVE;
  std::vector<std::uint8_t> reflectedValue;
  std::vector<std::uint8_t> reflectedTag;
  std::vector<char> callbackOrder;
};

void verifySharedCoordinatorLateArrival(bool requestAvailable) {
  EmbeddedFederationRegistry registry;
  auto service = std::make_shared<ProcessFederationService>(
      registry, composedDefinition());
  ProcessFederationServiceHost senderService(
      service,
      {"127.0.0.1", 0U},
      {"cross-service-tso-sender", 0xB831U});
  ProcessFederationServiceHost receiverService(
      service,
      {"127.0.0.1", 0U},
      {"cross-service-tso-receiver", 0xB832U});
  senderService.start();
  receiverService.start();

  constexpr wchar_t const* federationName =
      L"process-tso-cross-service-late-arrival";
  constexpr wchar_t const* federateType = L"process-tso-cross-service-type";
  constexpr wchar_t const* interactionName =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr wchar_t const* parameterName = L"TimelinessOk";

  ArrivalAmbassador senderAmbassador;
  ArrivalAmbassador receiverAmbassador;
  auto sender = makeRti();
  auto receiver = makeRti();
  REQUIRE(sender->connect(
              senderAmbassador,
              HLA_EVOKED,
              configurationFor(senderService.address(),
                               L"process-tso-cross-service-sender"))
              .addressUsed);
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName, L"server-owned-cross-service-fom.xml"));
  auto const senderHandle = sender->joinFederationExecution(
      L"process-tso-cross-service-sender", federateType, federationName);
  REQUIRE(senderHandle.isValid());

  REQUIRE(receiver->connect(
              receiverAmbassador,
              HLA_EVOKED,
              configurationFor(receiverService.address(),
                               L"process-tso-cross-service-receiver"))
              .addressUsed);
  auto const receiverHandle = receiver->joinFederationExecution(
      L"process-tso-cross-service-receiver", federateType, federationName);
  REQUIRE(receiverHandle.isValid());

  auto const senderClass = sender->getInteractionClassHandle(interactionName);
  auto const receiverClass = receiver->getInteractionClassHandle(interactionName);
  auto const senderParameter = sender->getParameterHandle(
      senderClass, parameterName);
  REQUIRE(senderClass == receiverClass);
  REQUIRE_NOTHROW(sender->publishInteractionClass(senderClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(receiverClass, true));
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  static_cast<void>(sender->evokeCallback(0.0));
  static_cast<void>(receiver->evokeCallback(0.0));

  if (requestAvailable) {
    REQUIRE_NOTHROW(receiver->nextMessageRequestAvailable(
        rti1516_2025::HLAinteger64Time(10)));
  } else {
    REQUIRE_NOTHROW(receiver->nextMessageRequest(
        rti1516_2025::HLAinteger64Time(10)));
  }
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverAmbassador.interactions == 0U);
  REQUIRE(receiverAmbassador.grants == 0U);

  std::uint8_t const payload = 0x61U;
  std::uint8_t const tag = 0x58U;
  ParameterHandleValueMap values;
  values.emplace(
      senderParameter,
      VariableLengthData(&payload, sizeof(payload)));
  auto const retraction = sender->sendInteraction(
      senderClass,
      values,
      VariableLengthData(&tag, sizeof(tag)),
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE_NOTHROW(sender->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(5)));

  REQUIRE(receiver->evokeCallback(0.0));
  REQUIRE(receiverAmbassador.callbackOrder == std::vector<char>{'I'});
  REQUIRE(receiverAmbassador.interactions == 1U);
  REQUIRE(receiverAmbassador.grants == 0U);
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverAmbassador.callbackOrder == std::vector<char>{'I', 'G'});
  REQUIRE(receiverAmbassador.interactions == 1U);
  REQUIRE(receiverAmbassador.grants == 1U);
  REQUIRE(receiverAmbassador.interactionTime == 5);
  REQUIRE(receiverAmbassador.grantTime == 5);
  REQUIRE(receiverAmbassador.producer == senderHandle);

  REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());

  senderService.stop();
  receiverService.stop();
  REQUIRE_NOTHROW(senderService.rethrowIfFailed());
  REQUIRE_NOTHROW(receiverService.rethrowIfFailed());
}

void verifySharedCoordinatorLateAttributeArrival(bool requestAvailable) {
  EmbeddedFederationRegistry registry;
  auto service = std::make_shared<ProcessFederationService>(
      registry, composedDefinition());
  ProcessFederationServiceHost senderService(
      service,
      {"127.0.0.1", 0U},
      {"process-tso-attribute-sender", 0xB833U});
  ProcessFederationServiceHost receiverService(
      service,
      {"127.0.0.1", 0U},
      {"process-tso-attribute-receiver", 0xB834U});
  senderService.start();
  receiverService.start();

  constexpr wchar_t const* federationName =
      L"process-tso-cross-service-attribute-arrival";
  constexpr wchar_t const* federateType =
      L"process-tso-cross-service-attribute-type";
  constexpr wchar_t const* objectClassName = L"HLAobjectRoot.Employee";
  constexpr wchar_t const* attributeName = L"Name";

  AttributeArrivalAmbassador senderAmbassador;
  AttributeArrivalAmbassador receiverAmbassador;
  auto sender = makeRti();
  auto receiver = makeRti();
  REQUIRE(sender->connect(
              senderAmbassador,
              HLA_EVOKED,
              configurationFor(senderService.address(),
                               L"process-tso-cross-service-attribute-sender"))
              .addressUsed);
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName, L"server-owned-cross-service-attribute-fom.xml"));
  auto const senderHandle = sender->joinFederationExecution(
      L"process-tso-cross-service-attribute-sender", federateType,
      federationName);
  REQUIRE(senderHandle.isValid());

  REQUIRE(receiver->connect(
              receiverAmbassador,
              HLA_EVOKED,
              configurationFor(receiverService.address(),
                               L"process-tso-cross-service-attribute-receiver"))
              .addressUsed);
  auto const receiverHandle = receiver->joinFederationExecution(
      L"process-tso-cross-service-attribute-receiver", federateType,
      federationName);
  REQUIRE(receiverHandle.isValid());

  auto const senderClass = sender->getObjectClassHandle(objectClassName);
  auto const receiverClass = receiver->getObjectClassHandle(objectClassName);
  auto const senderAttribute = sender->getAttributeHandle(
      senderClass, attributeName);
  auto const receiverAttribute = receiver->getAttributeHandle(
      receiverClass, attributeName);
  REQUIRE(senderClass == receiverClass);
  REQUIRE(senderAttribute == receiverAttribute);
  REQUIRE_NOTHROW(sender->publishObjectClassAttributes(
      senderClass, {senderAttribute}));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      receiverClass, {receiverAttribute}, true));
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  static_cast<void>(sender->evokeCallback(0.0));
  static_cast<void>(receiver->evokeCallback(0.0));

  auto const objectInstance = sender->registerObjectInstance(senderClass);
  REQUIRE(objectInstance.isValid());
  for (std::size_t attempt = 0U;
       attempt < 200U && !receiverAmbassador.discovered;
       ++attempt) {
    static_cast<void>(receiver->evokeCallback(0.01));
  }
  REQUIRE(receiverAmbassador.discovered);
  REQUIRE(receiverAmbassador.discoveredObject == objectInstance);
  REQUIRE(receiverAmbassador.callbackOrder == std::vector<char>{'D'});
  receiverAmbassador.callbackOrder.clear();

  if (requestAvailable) {
    REQUIRE_NOTHROW(receiver->nextMessageRequestAvailable(
        rti1516_2025::HLAinteger64Time(10)));
  } else {
    REQUIRE_NOTHROW(receiver->nextMessageRequest(
        rti1516_2025::HLAinteger64Time(10)));
  }
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverAmbassador.reflections == 0U);
  REQUIRE(receiverAmbassador.grants == 0U);

  std::array<std::uint8_t, 3U> const valueBytes{0x54U, 0x53U, 0x4FU};
  std::array<std::uint8_t, 3U> const tagBytes{0x41U, 0x54U, 0x54U};
  rti1516_2025::AttributeHandleValueMap values;
  values.emplace(
      senderAttribute,
      VariableLengthData(valueBytes.data(), valueBytes.size()));
  auto const retraction = sender->updateAttributeValues(
      objectInstance,
      values,
      VariableLengthData(tagBytes.data(), tagBytes.size()),
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE_NOTHROW(sender->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(5)));

  REQUIRE(receiver->evokeCallback(0.0));
  REQUIRE(receiverAmbassador.callbackOrder == std::vector<char>{'A'});
  REQUIRE(receiverAmbassador.reflections == 1U);
  REQUIRE(receiverAmbassador.grants == 0U);
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverAmbassador.callbackOrder == std::vector<char>{'A', 'G'});
  REQUIRE(receiverAmbassador.reflections == 1U);
  REQUIRE(receiverAmbassador.grants == 1U);
  REQUIRE(receiverAmbassador.reflectedObject == objectInstance);
  REQUIRE(receiverAmbassador.reflectionTime == 5);
  REQUIRE(receiverAmbassador.grantTime == 5);
  REQUIRE(receiverAmbassador.producer == senderHandle);
  REQUIRE(receiverAmbassador.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(receiverAmbassador.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(receiverAmbassador.reflectedValue ==
          std::vector<std::uint8_t>{0x54U, 0x53U, 0x4FU});
  REQUIRE(receiverAmbassador.reflectedTag ==
          std::vector<std::uint8_t>{0x41U, 0x54U, 0x54U});

  REQUIRE_NOTHROW(sender->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());

  senderService.stop();
  receiverService.stop();
  REQUIRE_NOTHROW(senderService.rethrowIfFailed());
  REQUIRE_NOTHROW(receiverService.rethrowIfFailed());
}

}  // namespace

TEST_CASE(
    "RTIambassadors coordinate late TSO arrivals across distinct listeners sharing one authoritative coordinator",
    "[integration][foundation][interaction-management][time-management]"
    "[time-advance][transport][process-boundary][public-endpoint]"
    "[process-tso-cross-service-late-arrival-coordination]"
    "[2025]") {
  SECTION("NMR receives a late TSO through the separate sender listener") {
    verifySharedCoordinatorLateArrival(false);
  }
  SECTION("NMRA receives a late TSO through the separate sender listener") {
    verifySharedCoordinatorLateArrival(true);
  }
}

TEST_CASE(
    "RTIambassadors reflect a late timestamped attribute update before the NMRA grant through shared coordinator endpoints",
    "[integration][foundation][object-management][time-management]"
    "[time-advance][transport][process-boundary][public-endpoint]"
    "[process-tso-shared-coordinator-late-arrival-attribute][2025]") {
  verifySharedCoordinatorLateAttributeArrival(true);
}

TEST_CASE(
    "RTIambassadors reflect a late timestamped attribute update before the NMR grant through shared coordinator endpoints",
    "[integration][foundation][object-management][time-management]"
    "[time-advance][transport][process-boundary][public-endpoint]"
    "[process-tso-shared-coordinator-late-arrival-attribute]"
    "[rti.service.update-attribute-values][rti.service.next-message-request]"
    "[federate.callback.reflect-attribute-values][federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-8.10.2][2025]") {
  verifySharedCoordinatorLateAttributeArrival(false);
}

#else

TEST_CASE(
    "RTIambassadors coordinate late TSO arrivals across distinct listeners sharing one authoritative coordinator",
    "[integration][foundation][interaction-management][time-management]"
    "[time-advance][transport][process-boundary][public-endpoint]"
    "[process-tso-cross-service-late-arrival-coordination]"
    "[2025]") {
  SUCCEED("Embedded federation management is disabled in this build.");
}

#endif
