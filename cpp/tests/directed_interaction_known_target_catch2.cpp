#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The directed interaction test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using TestFederateAmbassador = rti1516_2025::NullFederateAmbassador;
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

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"directed-interaction-known-target-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path testDataPath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct DirectedInteractionReport final {
    InteractionClassHandle interactionClass;
    ObjectInstanceHandle objectInstance;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
  }

  void receiveDirectedInteraction(
      InteractionClassHandle const& interactionClass,
      ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate) override {
    directedInteractionReports.push_back({
        interactionClass,
        objectInstance,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
    });
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const&,
      FederateHandle const&) override {
    objectRemovalReports.push_back(objectInstance);
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<DirectedInteractionReport> directedInteractionReports;
  std::vector<ObjectInstanceHandle> objectRemovalReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Embedded directed interactions route to known object-class subscribers",
    "[integration][development-profile][interaction-management][directed]"
    "[directed-target-departure]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.unpublish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.unsubscribe-object-class-directed-interactions]"
    "[rti.service.send-directed-interaction]"
    "[federate.callback.receive-directed-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador unsubscribedReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto immediate = makeRti();
  auto unsubscribed = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = testDataPath(
      "directed-interaction-object-consumer-fom.xml").wstring();
  auto const interactionProvider = testDataPath(
      "directed-interaction-interaction-provider-fom.xml").wstring();
  unsigned char const tagBytes[] = {0x43, 0x11, 0x9A};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  InteractionClassHandleSet directedClasses;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(unsubscribed->connect(unsubscribedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"directed-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"directed-subscriber",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"directed-immediate",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(unsubscribed->joinFederationExecution(
      L"directed-unsubscribed",
      L"subscriber",
      federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  directedClasses.insert(interactionClass);

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(unsubscribed->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      true));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      true));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(unsubscribed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(unsubscribedReports.objectDiscoveryReports.size() == 1U);

  // The target and interaction are valid, but only declared directed
  // subscribers receive the receive-order callback and the sender is excluded.
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  REQUIRE(subscriberReports.directedInteractionReports.empty());
  REQUIRE(immediateReports.directedInteractionReports.size() == 1U);
  REQUIRE(unsubscribedReports.directedInteractionReports.empty());
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.directedInteractionReports.size() == 1U);
  auto const& first = subscriberReports.directedInteractionReports.front();
  REQUIRE(first.interactionClass == interactionClass);
  REQUIRE(first.objectInstance == target);
  REQUIRE(first.parameterValues.empty());
  REQUIRE(variableLengthDataBytes(first.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(first.transportationType ==
          publisher->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE(first.producingFederate == publisherHandle);

  // An accepted queued callback is fenced by a subscription change. A fresh
  // subscription creates a new route instead of reviving stale work.
  subscriberReports.directedInteractionReports.clear();
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.directedInteractionReports.empty());
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      true));

  // The source publication is another callback-time fence. Unpublishing the
  // pair before evocation suppresses the already accepted send.
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.directedInteractionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->sendDirectedInteraction(
          interactionClass,
          target,
          ParameterHandleValueMap{},
          tag),
      rti1516_2025::InteractionClassNotPublished);

  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.directedInteractionReports.size() == 1U);

  // A target deletion accepted before callback dispatch makes the queued
  // directed work stale; the removal callback remains the sole target event.
  auto const directReportsBeforeTargetDeparture =
      subscriberReports.directedInteractionReports.size();
  VariableLengthData deletionTag;
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  REQUIRE_NOTHROW(publisher->deleteObjectInstance(target, deletionTag));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.directedInteractionReports.size() ==
          directReportsBeforeTargetDeparture);
  REQUIRE(subscriberReports.objectRemovalReports.size() == 1U);
  REQUIRE(subscriberReports.objectRemovalReports.front() == target);
  REQUIRE_THROWS_AS(
      publisher->sendDirectedInteraction(
          interactionClass,
          target,
          ParameterHandleValueMap{},
          tag),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(immediate->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(unsubscribed->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(unsubscribed->disconnect());
}
