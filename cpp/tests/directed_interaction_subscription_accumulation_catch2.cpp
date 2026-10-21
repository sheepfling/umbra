#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The directed-subscription accumulation test requires the Umbra source directory."
#endif

namespace {

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::InteractionClassHandleSet;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

inline constexpr wchar_t kTargetObjectClass[] =
    L"HLAobjectRoot.TckTypedObject.TckTypedDerivedObject";
inline constexpr wchar_t kTargetMarker[] = L"Identity";
inline constexpr wchar_t kFirstDirectedInteraction[] =
    L"HLAinteractionRoot.TckTypedInteraction";
inline constexpr wchar_t kSecondDirectedInteraction[] =
    L"HLAinteractionRoot.TckTypedAccumulationSiblingInteraction";

struct DirectedReceipt final {
  InteractionClassHandle interactionClass;
  ObjectInstanceHandle objectInstance;
};

class ReceiptFederateAmbassador final : public rti1516_2025::NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    discoveredObjects.push_back(objectInstance);
  }

  void receiveDirectedInteraction(
      InteractionClassHandle const& interactionClass,
      ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&) override {
    directedReceipts.push_back({interactionClass, objectInstance});
  }

  std::vector<ObjectInstanceHandle> discoveredObjects;
  std::vector<DirectedReceipt> directedReceipts;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"directed-interaction-subscription-accumulation-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path accumulationFom() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "packages" /
      "hla-rti-cpp-tck" / "fom" /
      "directed-interaction-subscription-accumulation-tck.xml";
}

void drainCallbacks(RTIambassador& ambassador) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!ambassador.evokeCallback(0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Embedded successive directed-interaction subscriptions accumulate for one object class",
    "[integration][development-profile][2025][interaction-management][directed]"
    "[directed-interaction-subscription-accumulation]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.send-directed-interaction]"
    "[federate.callback.receive-directed-interaction]") {
  ReceiptFederateAmbassador publisherCallbacks;
  ReceiptFederateAmbassador subscriberCallbacks;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const modelFom = accumulationFom().wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{modelFom},
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"accumulation-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"accumulation-subscriber", L"subscriber", federationName));

  auto const publisherObjectClass = publisher->getObjectClassHandle(kTargetObjectClass);
  auto const subscriberObjectClass = subscriber->getObjectClassHandle(kTargetObjectClass);
  auto const publisherMarker = publisher->getAttributeHandle(
      publisherObjectClass, kTargetMarker);
  auto const subscriberMarker = subscriber->getAttributeHandle(
      subscriberObjectClass, kTargetMarker);
  auto const publisherFirstInteraction =
      publisher->getInteractionClassHandle(kFirstDirectedInteraction);
  auto const publisherSecondInteraction =
      publisher->getInteractionClassHandle(kSecondDirectedInteraction);
  auto const subscriberFirstInteraction =
      subscriber->getInteractionClassHandle(kFirstDirectedInteraction);
  auto const subscriberSecondInteraction =
      subscriber->getInteractionClassHandle(kSecondDirectedInteraction);

  REQUIRE(publisherObjectClass.isValid());
  REQUIRE(subscriberObjectClass.isValid());
  REQUIRE(publisherMarker.isValid());
  REQUIRE(subscriberMarker.isValid());
  REQUIRE(publisherFirstInteraction.isValid());
  REQUIRE(publisherSecondInteraction.isValid());
  REQUIRE(subscriberFirstInteraction.isValid());
  REQUIRE(subscriberSecondInteraction.isValid());

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherObjectClass, {publisherMarker}));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      subscriberObjectClass, {subscriberMarker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      publisherObjectClass,
      InteractionClassHandleSet{publisherFirstInteraction, publisherSecondInteraction}));

  REQUIRE_NOTHROW(subscriber->subscribeObjectClassDirectedInteractions(
      subscriberObjectClass,
      InteractionClassHandleSet{subscriberFirstInteraction},
      true));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassDirectedInteractions(
      subscriberObjectClass,
      InteractionClassHandleSet{subscriberSecondInteraction},
      true));

  auto const target = publisher->registerObjectInstance(publisherObjectClass);
  REQUIRE(target.isValid());
  drainCallbacks(*subscriber);
  REQUIRE(subscriberCallbacks.discoveredObjects.size() == 1U);
  REQUIRE(subscriberCallbacks.discoveredObjects.front() == target);

  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      publisherFirstInteraction,
      target,
      ParameterHandleValueMap{},
      VariableLengthData{}));
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      publisherSecondInteraction,
      target,
      ParameterHandleValueMap{},
      VariableLengthData{}));
  drainCallbacks(*subscriber);

  REQUIRE(subscriberCallbacks.directedReceipts.size() == 2U);
  auto const firstWasReceived = std::any_of(
      subscriberCallbacks.directedReceipts.begin(),
      subscriberCallbacks.directedReceipts.end(),
      [&](DirectedReceipt const& receipt) {
        return receipt.interactionClass == subscriberFirstInteraction &&
            receipt.objectInstance == target;
      });
  auto const secondWasReceived = std::any_of(
      subscriberCallbacks.directedReceipts.begin(),
      subscriberCallbacks.directedReceipts.end(),
      [&](DirectedReceipt const& receipt) {
        return receipt.interactionClass == subscriberSecondInteraction &&
            receipt.objectInstance == target;
      });
  REQUIRE(firstWasReceived);
  REQUIRE(secondWasReceived);

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassDirectedInteractions(
      subscriberObjectClass));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassDirectedInteractions(
      publisherObjectClass));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(
      subscriberObjectClass, {subscriberMarker}));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      publisherObjectClass, {publisherMarker}));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
