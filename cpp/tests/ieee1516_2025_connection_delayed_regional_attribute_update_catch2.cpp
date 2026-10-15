#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service_host.hpp"
#include "internal/fom/fom_validation.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

using namespace rti1516_2025;
using namespace umbra::detail;

std::filesystem::path resource(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relative;
}

PrevalidatedFomModule validateModule(
    std::filesystem::path const& path,
    FomModuleKind kind,
    std::wstring designator) {
  LibXml2FomValidator validator;
  auto result = validator.validate({
      path,
      resource("schemas/IEEE1516-DIF-2025.xsd"),
      kind,
      std::move(designator),
      L"IEEE1516-DIF-2025.xsd",
  });
  if (result.status != FomValidationStatus::valid || !result.module) {
    throw std::runtime_error("The delayed regional process FOM did not validate.");
  }
  return *result.module;
}

FederationDefinition delayedRegionalDefinition(bool enableDelay) {
  std::vector<PrevalidatedFomModule> modules{
      validateModule(
          resource("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:delayed-regional-process-mim"),
      validateModule(
          resource("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:delayed-regional-process-restaurant"),
  };
  if (enableDelay) {
    modules.push_back(validateModule(
        std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
            "data" / "delay-subscription-evaluation-enabled-fom.xml",
        FomModuleKind::fom,
        L"urn:umbra:test:delayed-regional-process-switches"));
  }
  LibXml2FomModuleComposer composer(resource("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid || !result.catalog ||
      !result.fdd) {
    throw std::runtime_error("The delayed regional process FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

class RegionalReceiver final : public NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const&,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    ++discoveries;
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const&,
      AttributeHandleValueMap const& values,
      VariableLengthData const& tag,
      TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const* sentRegions) override {
    if (onReflection) {
      onReflection();
    }
    receivedAttributes = values.size();
    receivedSentRegions = sentRegions == nullptr ? 0U : sentRegions->size();
    if (tag.size() != 0U) {
      auto const* bytes = static_cast<std::uint8_t const*>(tag.data());
      receivedTag.assign(bytes, bytes + tag.size());
    }
    receivedTags.push_back(receivedTag);
    reflections.fetch_add(1U, std::memory_order_release);
  }

  std::atomic<std::size_t> discoveries = 0U;
  std::atomic<std::size_t> reflections = 0U;
  std::size_t receivedAttributes = 0U;
  std::size_t receivedSentRegions = 0U;
  std::vector<std::uint8_t> receivedTag;
  std::vector<std::vector<std::uint8_t>> receivedTags;
  std::function<void()> onReflection;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

RtiConfiguration configurationFor(std::uint16_t port, wchar_t const* name) {
  return RtiConfiguration::createConfiguration()
      .withConfigurationName(name)
      .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
}

void drainCallbacks(RTIambassador& rti) {
  while (rti.evokeCallback(0.0)) {
  }
}

void drainCallbacksFor(RTIambassador& rti, CallbackModel callbackModel) {
  for (unsigned attempt = 0U; attempt < 20U; ++attempt) {
    if (callbackModel == HLA_EVOKED) {
      drainCallbacks(rti);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
}

template <typename Ready>
void pumpCallbacksUntil(RTIambassador& rti, CallbackModel callbackModel, Ready ready) {
  for (unsigned attempt = 0U; attempt < 50U && !ready(); ++attempt) {
    if (callbackModel == HLA_EVOKED) {
      drainCallbacks(rti);
    }
    if (!ready()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
  }
}

void runScenario(bool enableDelay, CallbackModel callbackModel) {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost host(
      registry,
      delayedRegionalDefinition(enableDelay),
      {"127.0.0.1", 0U},
      {"umbra-delayed-regional-process-test", 0xD351U},
      ProcessFederationServiceOptions{callbackModel == HLA_IMMEDIATE});
  host.start();

  constexpr wchar_t const* federationName = L"process-delayed-regional-attribute";
  NullFederateAmbassador senderFederate;
  RegionalReceiver receiverFederate;
  auto sender = makeRti();
  auto receiver = makeRti();
  REQUIRE(sender->connect(
      senderFederate,
      callbackModel,
      configurationFor(host.address().port, L"delayed-regional-sender")).addressUsed);
  REQUIRE(receiver->connect(
      receiverFederate,
      callbackModel,
      configurationFor(host.address().port, L"delayed-regional-receiver")).addressUsed);
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName, L"server-owned-fom.xml"));
  REQUIRE_NOTHROW(sender->joinFederationExecution(L"sender", L"test", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", L"test", federationName));
  auto const receiverMembership = registry.memberByName(federationName, L"receiver");
  REQUIRE(receiverMembership);
  REQUIRE(registry.delaySubscriptionEvaluationSwitchFor(
              federationName, receiverMembership->id) == enableDelay);

  auto const senderClass = sender->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const senderAttribute = sender->getAttributeHandle(senderClass, L"Flavor");
  auto const senderDimension = sender->getDimensionHandle(L"SodaFlavor");
  auto const receiverClass = receiver->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const receiverAttribute = receiver->getAttributeHandle(receiverClass, L"Flavor");
  auto const receiverDimension = receiver->getDimensionHandle(L"SodaFlavor");
  REQUIRE(senderClass.isValid());
  REQUIRE(senderAttribute.isValid());
  REQUIRE(senderDimension.isValid());
  REQUIRE(receiverClass.isValid());
  REQUIRE(receiverAttribute.isValid());
  REQUIRE(receiverDimension.isValid());
  auto const senderRegion = sender->createRegion(DimensionHandleSet{senderDimension});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{receiverDimension});
  sender->setRangeBounds(senderRegion, senderDimension, RangeBounds(0UL, 1UL));
  sender->commitRegionModifications(RegionHandleSet{senderRegion});
  receiver->setRangeBounds(receiverRegion, receiverDimension, RangeBounds(0UL, 1UL));
  receiver->commitRegionModifications(RegionHandleSet{receiverRegion});
  AttributeHandleSet const senderAttributes{senderAttribute};
  AttributeHandleSetRegionHandleSetPairVector const senderPairs{{
      senderAttributes,
      RegionHandleSet{senderRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPairs{{
      AttributeHandleSet{receiverAttribute},
      RegionHandleSet{receiverRegion},
  }};
  sender->publishObjectClassAttributes(senderClass, senderAttributes);
  receiver->subscribeObjectClassAttributesWithRegions(receiverClass, receiverPairs);
  auto const object = sender->registerObjectInstanceWithRegions(senderClass, senderPairs);
  REQUIRE(object.isValid());
  pumpCallbacksUntil(*receiver, callbackModel, [&] {
    return receiverFederate.discoveries == 1U;
  });
  REQUIRE(receiverFederate.discoveries == 1U);
  if (callbackModel == HLA_IMMEDIATE) {
    receiver->disableCallbacks();
  }

  // The receiver is known, but has no attribute declaration when each update
  // is generated. The creation-time switch keeps its route until delivery.
  receiver->unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPairs);
  receiver->setRangeBounds(receiverRegion, receiverDimension, RangeBounds(2UL, 3UL));
  receiver->commitRegionModifications(RegionHandleSet{receiverRegion});
  std::array<std::uint8_t, 1U> const value{0xD5U};
  AttributeHandleValueMap values;
  values.emplace(senderAttribute, VariableLengthData(value.data(), value.size()));
  std::array<std::uint8_t, 1U> const firstTag{0xD1U};
  sender->updateAttributeValues(
      object, values, VariableLengthData(firstTag.data(), firstTag.size()));
  receiver->subscribeObjectClassAttributesWithRegions(receiverClass, receiverPairs);
  if (callbackModel == HLA_IMMEDIATE) {
    receiver->enableCallbacks();
  }
  drainCallbacksFor(*receiver, callbackModel);
  REQUIRE(receiverFederate.reflections == 0U);

  if (callbackModel == HLA_IMMEDIATE) {
    receiver->disableCallbacks();
  }
  receiver->unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPairs);
  receiver->setRangeBounds(receiverRegion, receiverDimension, RangeBounds(0UL, 1UL));
  receiver->commitRegionModifications(RegionHandleSet{receiverRegion});
  std::array<std::uint8_t, 1U> const secondTag{0xD2U};
  sender->updateAttributeValues(
      object, values, VariableLengthData(secondTag.data(), secondTag.size()));
  // Eligibility at receive uses the source's committed range when Update was
  // issued, not a later mutation of the same region handle.
  sender->setRangeBounds(senderRegion, senderDimension, RangeBounds(2UL, 3UL));
  sender->commitRegionModifications(RegionHandleSet{senderRegion});
  receiver->subscribeObjectClassAttributesWithRegions(receiverClass, receiverPairs);
  if (callbackModel == HLA_IMMEDIATE) {
    receiver->enableCallbacks();
  }
  if (enableDelay) {
    pumpCallbacksUntil(*receiver, callbackModel, [&] {
      return receiverFederate.reflections == 1U;
    });
  } else {
    drainCallbacksFor(*receiver, callbackModel);
  }
  REQUIRE(receiverFederate.reflections == (enableDelay ? 1U : 0U));
  if (enableDelay) {
    REQUIRE(receiverFederate.receivedAttributes == 1U);
    REQUIRE(receiverFederate.receivedSentRegions == 1U);
    REQUIRE(receiverFederate.receivedTag ==
            std::vector<std::uint8_t>(secondTag.begin(), secondTag.end()));
  }

  // A passel initially eligible for this overlapping declaration must still
  // be suppressed when the committed receiving range becomes disjoint before
  // callback entry, irrespective of the switch setting.
  if (callbackModel == HLA_IMMEDIATE) {
    receiver->disableCallbacks();
  }
  sender->setRangeBounds(senderRegion, senderDimension, RangeBounds(0UL, 1UL));
  sender->commitRegionModifications(RegionHandleSet{senderRegion});
  std::array<std::uint8_t, 1U> const thirdTag{0xD3U};
  sender->updateAttributeValues(
      object, values, VariableLengthData(thirdTag.data(), thirdTag.size()));
  receiver->setRangeBounds(receiverRegion, receiverDimension, RangeBounds(2UL, 3UL));
  receiver->commitRegionModifications(RegionHandleSet{receiverRegion});
  if (callbackModel == HLA_IMMEDIATE) {
    receiver->enableCallbacks();
  }
  drainCallbacksFor(*receiver, callbackModel);
  REQUIRE(receiverFederate.reflections == (enableDelay ? 1U : 0U));

  if (callbackModel == HLA_IMMEDIATE) {
    // Two readiness markers wait behind one disabled callback gate. The
    // first passel is generated while the current receiver range is
    // disjoint; only the enabled switch may admit it after the late overlap.
    // The second is initially eligible in either mode. Preserve their FIFO
    // order without allowing a suppressed slot to claim its successor.
    receiver->disableCallbacks();
    std::array<std::uint8_t, 1U> const fourthTag{0xD4U};
    sender->updateAttributeValues(
        object, values, VariableLengthData(fourthTag.data(), fourthTag.size()));
    receiver->setRangeBounds(receiverRegion, receiverDimension, RangeBounds(0UL, 1UL));
    receiver->commitRegionModifications(RegionHandleSet{receiverRegion});
    std::array<std::uint8_t, 1U> const fifthTag{0xD5U};
    sender->updateAttributeValues(
        object, values, VariableLengthData(fifthTag.data(), fifthTag.size()));
    receiver->enableCallbacks();
    auto const expectedReflections = enableDelay ? 3U : 1U;
    pumpCallbacksUntil(*receiver, callbackModel, [&] {
      return receiverFederate.reflections == expectedReflections;
    });
    REQUIRE(receiverFederate.reflections == expectedReflections);
    if (enableDelay) {
      REQUIRE(receiverFederate.receivedTags ==
              std::vector<std::vector<std::uint8_t>>{{0xD2U}, {0xD4U}, {0xD5U}});
    } else {
      REQUIRE(receiverFederate.receivedTags ==
              std::vector<std::vector<std::uint8_t>>{{0xD5U}});
    }

    // Object deletion after acceptance but before callback entry invalidates
    // this passel; the service still consumes its matching readiness marker.
    receiver->disableCallbacks();
    std::array<std::uint8_t, 1U> const sixthTag{0xD6U};
    sender->updateAttributeValues(
        object, values, VariableLengthData(sixthTag.data(), sixthTag.size()));
    sender->deleteObjectInstance(object, VariableLengthData{});
    auto const sentinel = sender->registerObjectInstanceWithRegions(
        senderClass, senderPairs);
    REQUIRE(sentinel.isValid());
    std::array<std::uint8_t, 1U> const seventhTag{0xD7U};
    sender->updateAttributeValues(
        sentinel, values, VariableLengthData(seventhTag.data(), seventhTag.size()));
    receiver->enableCallbacks();
    pumpCallbacksUntil(*receiver, callbackModel, [&] {
      return receiverFederate.reflections == expectedReflections + 1U;
    });
    REQUIRE(receiverFederate.reflections == expectedReflections + 1U);
    REQUIRE(receiverFederate.receivedTag ==
            std::vector<std::uint8_t>(seventhTag.begin(), seventhTag.end()));
  }

  receiver->resignFederationExecution(NO_ACTION);
  sender->resignFederationExecution(DELETE_OBJECTS);
  sender->destroyFederationExecution(federationName);
  receiver->disconnect();
  sender->disconnect();
  host.stop();
  REQUIRE_NOTHROW(host.rethrowIfFailed());
}

void runResignCallbackFenceScenario() {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost host(
      registry,
      delayedRegionalDefinition(true),
      {"127.0.0.1", 0U},
      {"umbra-delayed-regional-resign-test", 0xD352U},
      ProcessFederationServiceOptions{true});
  host.start();

  constexpr wchar_t const* federationName = L"process-delayed-regional-resign";
  NullFederateAmbassador senderFederate;
  RegionalReceiver receiverFederate;
  auto sender = makeRti();
  auto receiver = makeRti();
  REQUIRE(sender->connect(
      senderFederate, HLA_IMMEDIATE,
      configurationFor(host.address().port, L"delayed-regional-resign-sender")).addressUsed);
  REQUIRE(receiver->connect(
      receiverFederate, HLA_IMMEDIATE,
      configurationFor(host.address().port, L"delayed-regional-resign-receiver")).addressUsed);
  sender->createFederationExecution(federationName, L"server-owned-fom.xml");
  sender->joinFederationExecution(L"sender", L"test", federationName);
  receiver->joinFederationExecution(L"receiver", L"test", federationName);
  auto const oldMember = registry.memberByName(federationName, L"receiver");
  REQUIRE(oldMember);

  auto const objectClass = sender->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const attribute = sender->getAttributeHandle(objectClass, L"Flavor");
  auto const dimension = sender->getDimensionHandle(L"SodaFlavor");
  auto const senderRegion = sender->createRegion(DimensionHandleSet{dimension});
  sender->setRangeBounds(senderRegion, dimension, RangeBounds(0UL, 1UL));
  sender->commitRegionModifications(RegionHandleSet{senderRegion});
  AttributeHandleSet const attributes{attribute};
  AttributeHandleSetRegionHandleSetPairVector const senderPairs{{
      attributes, RegionHandleSet{senderRegion},
  }};
  sender->publishObjectClassAttributes(objectClass, attributes);

  auto const receiverClass = receiver->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const receiverAttribute = receiver->getAttributeHandle(receiverClass, L"Flavor");
  auto const receiverDimension = receiver->getDimensionHandle(L"SodaFlavor");
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{receiverDimension});
  receiver->setRangeBounds(receiverRegion, receiverDimension, RangeBounds(0UL, 1UL));
  receiver->commitRegionModifications(RegionHandleSet{receiverRegion});
  AttributeHandleSetRegionHandleSetPairVector const receiverPairs{{
      AttributeHandleSet{receiverAttribute}, RegionHandleSet{receiverRegion},
  }};
  receiver->subscribeObjectClassAttributesWithRegions(receiverClass, receiverPairs);
  auto const object = sender->registerObjectInstanceWithRegions(objectClass, senderPairs);
  REQUIRE(object.isValid());
  pumpCallbacksUntil(*receiver, HLA_IMMEDIATE, [&] {
    return receiverFederate.discoveries == 1U;
  });
  REQUIRE(receiverFederate.discoveries == 1U);

  std::mutex gateMutex;
  std::condition_variable gate;
  bool entered = false;
  bool release = false;
  std::atomic<bool> blockFirst{true};
  receiverFederate.onReflection = [&] {
    if (!blockFirst.exchange(false)) {
      return;
    }
    std::unique_lock lock(gateMutex);
    entered = true;
    gate.notify_all();
    gate.wait(lock, [&] { return release; });
  };
  std::array<std::uint8_t, 1U> const value{0xE0U};
  AttributeHandleValueMap values;
  values.emplace(attribute, VariableLengthData(value.data(), value.size()));
  std::array<std::uint8_t, 1U> const oldTag{0xE1U};
  sender->updateAttributeValues(
      object, values, VariableLengthData(oldTag.data(), oldTag.size()));
  {
    std::unique_lock lock(gateMutex);
    REQUIRE(gate.wait_for(lock, std::chrono::seconds(2), [&] { return entered; }));
  }
  std::array<std::uint8_t, 1U> const queuedOldTag{0xE2U};
  sender->updateAttributeValues(
      object, values, VariableLengthData(queuedOldTag.data(), queuedOldTag.size()));

  std::atomic<bool> resignFinished{false};
  std::exception_ptr resignError;
  std::thread resignThread([&] {
    try {
      receiver->resignFederationExecution(NO_ACTION);
    } catch (...) {
      resignError = std::current_exception();
    }
    resignFinished.store(true, std::memory_order_release);
  });
  bool serverResigned = false;
  for (unsigned attempt = 0U; attempt < 200U; ++attempt) {
    if (!registry.memberByName(federationName, L"receiver")) {
      serverResigned = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  auto const completedBeforeCallbackReturn =
      resignFinished.load(std::memory_order_acquire);
  {
    std::scoped_lock lock(gateMutex);
    release = true;
  }
  gate.notify_all();
  resignThread.join();
  REQUIRE(serverResigned);
  REQUIRE_FALSE(completedBeforeCallbackReturn);
  if (resignError) {
    std::rethrow_exception(resignError);
  }
  REQUIRE(receiverFederate.reflections == 1U);
  REQUIRE(receiverFederate.receivedTag ==
          std::vector<std::uint8_t>(oldTag.begin(), oldTag.end()));

  receiver->joinFederationExecution(L"receiver", L"test", federationName);
  auto const newMember = registry.memberByName(federationName, L"receiver");
  REQUIRE(newMember);
  REQUIRE(newMember->id != oldMember->id);
  auto const newRegion = receiver->createRegion(DimensionHandleSet{receiverDimension});
  receiver->setRangeBounds(newRegion, receiverDimension, RangeBounds(0UL, 1UL));
  receiver->commitRegionModifications(RegionHandleSet{newRegion});
  AttributeHandleSetRegionHandleSetPairVector const newPairs{{
      AttributeHandleSet{receiverAttribute}, RegionHandleSet{newRegion},
  }};
  receiver->subscribeObjectClassAttributesWithRegions(receiverClass, newPairs);
  std::array<std::uint8_t, 1U> const newTag{0xE3U};
  sender->updateAttributeValues(
      object, values, VariableLengthData(newTag.data(), newTag.size()));
  pumpCallbacksUntil(*receiver, HLA_IMMEDIATE, [&] {
    return receiverFederate.reflections == 2U;
  });
  REQUIRE(receiverFederate.reflections == 2U);
  REQUIRE(receiverFederate.receivedTags ==
          std::vector<std::vector<std::uint8_t>>{{0xE1U}, {0xE3U}});

  receiver->resignFederationExecution(NO_ACTION);
  sender->resignFederationExecution(DELETE_OBJECTS);
  sender->destroyFederationExecution(federationName);
  receiver->disconnect();
  sender->disconnect();
  host.stop();
  REQUIRE_NOTHROW(host.rethrowIfFailed());
}

}  // namespace

TEST_CASE(
    "Process delayed regional attribute delivery rechecks the original sent region",
    "[integration][object-management][data-distribution-management]"
    "[delay-subscription-evaluation][process-boundary]"
    "[process-delayed-regional-attribute][2025]"
    "[rti.service.update-attribute-values]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[federate.callback.reflect-attribute-values]") {
  SECTION("enabled delays eligibility to the HLA_EVOKED receive fence") {
    runScenario(true, HLA_EVOKED);
  }
  SECTION("disabled preserves generation-time exclusion") {
    runScenario(false, HLA_EVOKED);
  }
}

TEST_CASE(
    "Process immediate delayed regional callback completes before resign returns",
    "[integration][object-management][data-distribution-management]"
    "[delay-subscription-evaluation][process-boundary]"
    "[process-deferred-regional-resign-fence][2025]"
    "[rti.service.resign-federation-execution]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.reflect-attribute-values]") {
  runResignCallbackFenceScenario();
}

TEST_CASE(
    "Process immediate delayed regional attribute delivery rechecks at callback entry",
    "[integration][object-management][data-distribution-management]"
    "[delay-subscription-evaluation][process-boundary]"
    "[process-delayed-regional-attribute-immediate][2025]"
    "[rti.service.update-attribute-values]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[federate.callback.reflect-attribute-values]") {
  SECTION("enabled defers eligibility to the HLA_IMMEDIATE callback entry") {
    runScenario(true, HLA_IMMEDIATE);
  }
  SECTION("disabled preserves generation-time exclusion") {
    runScenario(false, HLA_IMMEDIATE);
  }
}
