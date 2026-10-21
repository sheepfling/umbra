#include <catch2/catch_test_macros.hpp>

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
#include <string>
#include <thread>
#include <vector>

#ifndef UMBRA_PACKAGE_DELAYED_REGIONAL_MIM_FOM
#error "The package delayed-regional test requires the official 2025 MIM fixture."
#endif
#ifndef UMBRA_PACKAGE_DELAYED_REGIONAL_RESTAURANT_FOM
#error "The package delayed-regional test requires the 2025 Restaurant FOM fixture."
#endif
#ifndef UMBRA_PACKAGE_DELAYED_REGIONAL_SWITCH_FOM
#error "The package delayed-regional test requires the DSE switch fixture."
#endif

namespace {

using namespace rti1516_2025;

constexpr wchar_t kObjectClassName[] = L"HLAobjectRoot.Food.Drink.Soda";
constexpr wchar_t kAttributeName[] = L"Flavor";
constexpr wchar_t kDimensionName[] = L"SodaFlavor";

struct Reflection final {
  std::vector<std::uint8_t> tag;
  std::size_t attributeCount = 0U;
};

class Reports final : public NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const&,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    discoveries.fetch_add(1U, std::memory_order_release);
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const&,
      AttributeHandleValueMap const& values,
      VariableLengthData const& tag,
      TransportationTypeHandle const&,
      FederateHandle const&,
    RegionHandleSet const*) override {
    Reflection reflection;
    reflection.attributeCount = values.size();
    if (tag.size() != 0U) {
      auto const* bytes = static_cast<std::uint8_t const*>(tag.data());
      reflection.tag.assign(bytes, bytes + tag.size());
    }
    {
      std::scoped_lock lock(mutex);
      reflections.push_back(std::move(reflection));
    }
    if (onReflection) {
      onReflection();
    }
    reflectionCount.fetch_add(1U, std::memory_order_release);
  }

  std::vector<Reflection> snapshot() const {
    std::scoped_lock lock(mutex);
    return reflections;
  }

  std::atomic<std::size_t> discoveries{0U};
  std::atomic<std::size_t> reflectionCount{0U};
  std::function<void()> onReflection;

 private:
  mutable std::mutex mutex;
  std::vector<Reflection> reflections;
};

std::wstring uniqueFederationName(wchar_t const* prefix) {
  static std::atomic_uint64_t sequence{0U};
  return std::wstring(prefix) +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void createDseFederation(
    RTIambassador& creator,
    std::wstring const& federationName) {
  creator.createFederationExecutionWithMIM(
      federationName,
      std::vector<std::wstring>{
          std::filesystem::path(UMBRA_PACKAGE_DELAYED_REGIONAL_RESTAURANT_FOM)
              .wstring(),
          std::filesystem::path(UMBRA_PACKAGE_DELAYED_REGIONAL_SWITCH_FOM)
              .wstring()},
      std::filesystem::path(UMBRA_PACKAGE_DELAYED_REGIONAL_MIM_FOM).wstring(),
      L"HLAinteger64Time");
}

bool waitUntil(std::function<bool()> const& ready) {
  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::seconds(3);
  while (std::chrono::steady_clock::now() < deadline) {
    if (ready()) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return ready();
}

struct RegionalHandles final {
  ObjectClassHandle senderClass;
  AttributeHandle senderAttribute;
  DimensionHandle senderDimension;
  ObjectClassHandle receiverClass;
  AttributeHandle receiverAttribute;
  DimensionHandle receiverDimension;
  RegionHandle senderRegion;
  RegionHandle receiverRegion;
  AttributeHandleSetRegionHandleSetPairVector senderPairs;
  AttributeHandleSetRegionHandleSetPairVector receiverPairs;
  ObjectInstanceHandle object;
};

RegionalHandles prepareRegionalObject(
    RTIambassador& sender,
    RTIambassador& receiver) {
  RegionalHandles handles;
  handles.senderClass = sender.getObjectClassHandle(kObjectClassName);
  handles.senderAttribute =
      sender.getAttributeHandle(handles.senderClass, kAttributeName);
  handles.senderDimension = sender.getDimensionHandle(kDimensionName);
  handles.receiverClass = receiver.getObjectClassHandle(kObjectClassName);
  handles.receiverAttribute =
      receiver.getAttributeHandle(handles.receiverClass, kAttributeName);
  handles.receiverDimension = receiver.getDimensionHandle(kDimensionName);
  REQUIRE(handles.senderClass.isValid());
  REQUIRE(handles.senderAttribute.isValid());
  REQUIRE(handles.senderDimension.isValid());
  REQUIRE(handles.receiverClass.isValid());
  REQUIRE(handles.receiverAttribute.isValid());
  REQUIRE(handles.receiverDimension.isValid());

  handles.senderRegion =
      sender.createRegion(DimensionHandleSet{handles.senderDimension});
  handles.receiverRegion =
      receiver.createRegion(DimensionHandleSet{handles.receiverDimension});
  sender.setRangeBounds(
      handles.senderRegion, handles.senderDimension, RangeBounds(0UL, 1UL));
  sender.commitRegionModifications(RegionHandleSet{handles.senderRegion});
  receiver.setRangeBounds(
      handles.receiverRegion, handles.receiverDimension, RangeBounds(0UL, 1UL));
  receiver.commitRegionModifications(RegionHandleSet{handles.receiverRegion});
  AttributeHandleSet const senderAttributes{handles.senderAttribute};
  handles.senderPairs.push_back(
      {senderAttributes, RegionHandleSet{handles.senderRegion}});
  handles.receiverPairs.push_back(
      {AttributeHandleSet{handles.receiverAttribute},
       RegionHandleSet{handles.receiverRegion}});
  sender.publishObjectClassAttributes(handles.senderClass, senderAttributes);
  receiver.subscribeObjectClassAttributesWithRegions(
      handles.receiverClass, handles.receiverPairs);
  handles.object =
      sender.registerObjectInstanceWithRegions(handles.senderClass, handles.senderPairs);
  REQUIRE(handles.object.isValid());
  return handles;
}

void sendTaggedUpdate(
    RTIambassador& sender,
    ObjectInstanceHandle const& object,
    AttributeHandle const& attribute,
    std::uint8_t tagValue) {
  std::array<std::uint8_t, 1U> const value{tagValue};
  std::array<std::uint8_t, 1U> const tag{tagValue};
  sender.updateAttributeValues(
      object,
      AttributeHandleValueMap{{
          attribute, VariableLengthData(value.data(), value.size())}},
      VariableLengthData(tag.data(), tag.size()));
}

void runDseProjection() {
  auto const federationName = uniqueFederationName(L"package-dse-regional-");
  Reports senderReports;
  Reports receiverReports;
  auto sender = makeRti();
  auto receiver = makeRti();
  sender->connect(senderReports, HLA_IMMEDIATE);
  receiver->connect(receiverReports, HLA_IMMEDIATE);
  createDseFederation(*sender, federationName);
  sender->joinFederationExecution(L"package-dse-sender", L"publisher", federationName);
  receiver->joinFederationExecution(L"package-dse-receiver", L"subscriber", federationName);
  auto const handles = prepareRegionalObject(*sender, *receiver);
  REQUIRE(waitUntil([&] {
    return receiverReports.discoveries.load(std::memory_order_acquire) == 1U;
  }));

  // Hold callback-task admission while a passel is generated, then change the
  // subscription and source region. Assert the late-overlap delivery outcome;
  // do not infer optional sent-region callback metadata from this projection.
  receiver->disableCallbacks();
  receiver->unsubscribeObjectClassAttributesWithRegions(
      handles.receiverClass, handles.receiverPairs);
  receiver->setRangeBounds(
      handles.receiverRegion,
      handles.receiverDimension,
      RangeBounds(2UL, 3UL));
  receiver->commitRegionModifications(RegionHandleSet{handles.receiverRegion});
  sendTaggedUpdate(*sender, handles.object, handles.senderAttribute, 0xD1U);
  sender->setRangeBounds(
      handles.senderRegion, handles.senderDimension, RangeBounds(2UL, 3UL));
  sender->commitRegionModifications(RegionHandleSet{handles.senderRegion});
  receiver->setRangeBounds(
      handles.receiverRegion,
      handles.receiverDimension,
      RangeBounds(0UL, 1UL));
  receiver->commitRegionModifications(RegionHandleSet{handles.receiverRegion});
  receiver->subscribeObjectClassAttributesWithRegions(
      handles.receiverClass, handles.receiverPairs);
  receiver->enableCallbacks();

  REQUIRE(waitUntil([&] {
    return receiverReports.reflectionCount.load(std::memory_order_acquire) == 1U;
  }));
  auto const reflections = receiverReports.snapshot();
  REQUIRE(reflections.size() == 1U);
  CHECK(reflections.front().attributeCount == 1U);
  CHECK(reflections.front().tag == std::vector<std::uint8_t>{0xD1U});

  receiver->resignFederationExecution(NO_ACTION);
  sender->resignFederationExecution(DELETE_OBJECTS);
  sender->destroyFederationExecution(federationName);
  receiver->disconnect();
  sender->disconnect();
}

void runResignCallbackFenceProjection() {
  auto const federationName = uniqueFederationName(L"package-resign-fence-");
  Reports senderReports;
  Reports receiverReports;
  auto sender = makeRti();
  auto receiver = makeRti();
  sender->connect(senderReports, HLA_IMMEDIATE);
  receiver->connect(receiverReports, HLA_IMMEDIATE);
  createDseFederation(*sender, federationName);
  sender->joinFederationExecution(L"package-fence-sender", L"publisher", federationName);
  receiver->joinFederationExecution(L"package-fence-receiver", L"subscriber", federationName);
  auto const handles = prepareRegionalObject(*sender, *receiver);
  REQUIRE(waitUntil([&] {
    return receiverReports.discoveries.load(std::memory_order_acquire) == 1U;
  }));

  std::mutex gateMutex;
  std::condition_variable gate;
  bool callbackEntered = false;
  bool releaseCallback = false;
  receiverReports.onReflection = [&] {
    std::unique_lock lock(gateMutex);
    callbackEntered = true;
    gate.notify_all();
    gate.wait(lock, [&] { return releaseCallback; });
  };
  std::exception_ptr sendError;
  std::thread sendThread([&] {
    try {
      sendTaggedUpdate(*sender, handles.object, handles.senderAttribute, 0xE1U);
    } catch (...) {
      sendError = std::current_exception();
    }
  });
  bool callbackEnteredInTime = false;
  {
    std::unique_lock lock(gateMutex);
    callbackEnteredInTime = gate.wait_for(lock, std::chrono::seconds(2), [&] {
      return callbackEntered;
    });
  }
  if (!callbackEnteredInTime) {
    {
      std::scoped_lock lock(gateMutex);
      releaseCallback = true;
    }
    gate.notify_all();
    sendThread.join();
    if (sendError) {
      std::rethrow_exception(sendError);
    }
    FAIL("the reflection callback did not enter before the deadline");
  }
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
  std::this_thread::sleep_for(std::chrono::milliseconds(40));
  bool const resignReturnedBeforeCallback =
      resignFinished.load(std::memory_order_acquire);
  {
    std::scoped_lock lock(gateMutex);
    releaseCallback = true;
  }
  gate.notify_all();
  resignThread.join();
  sendThread.join();
  if (sendError) {
    std::rethrow_exception(sendError);
  }
  REQUIRE_FALSE(resignReturnedBeforeCallback);
  if (resignError) {
    std::rethrow_exception(resignError);
  }
  REQUIRE(receiverReports.reflectionCount.load(std::memory_order_acquire) == 1U);
  sender->resignFederationExecution(DELETE_OBJECTS);
  sender->destroyFederationExecution(federationName);
  receiver->disconnect();
  sender->disconnect();
}

}  // namespace

TEST_CASE(
    "Installed 2025 package admits a delayed regional reflection after overlapping resubscription",
    "[installable-package][2025][data-distribution-management]"
    "[package-delayed-regional-attribute-immediate]"
    "[rti.service.update-attribute-values]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[federate.callback.reflect-attribute-values]") {
  runDseProjection();
}

TEST_CASE(
    "Installed 2025 package fences an active callback before resign returns",
    "[installable-package][2025][internal][package-resign-callback-fence]"
    "[rti.service.update-attribute-values]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.reflect-attribute-values]") {
  runResignCallbackFenceProjection();
}
