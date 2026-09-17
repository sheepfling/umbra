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
#error "The directed subscription-kind test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

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
  return L"directed-interaction-subscription-kind-" +
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

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<DirectedInteractionReport> directedInteractionReports;
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
    "Embedded directed interactions distinguish ownership and universal subscriptions",
    "[integration][development-profile][interaction-management][directed][ownership]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.send-directed-interaction]"
    "[federate.callback.receive-directed-interaction]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador senderReports;
  ReportingFederateAmbassador ownershipSubscriberReports;
  ReportingFederateAmbassador universalSubscriberReports;
  auto owner = makeRti();
  auto sender = makeRti();
  auto ownershipSubscriber = makeRti();
  auto universalSubscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = testDataPath(
      "directed-interaction-object-consumer-fom.xml").wstring();
  auto const interactionProvider = testDataPath(
      "directed-interaction-interaction-provider-fom.xml").wstring();
  unsigned char const tagBytes[] = {0x55, 0x4E, 0x49};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(ownershipSubscriber->connect(ownershipSubscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(universalSubscriber->connect(universalSubscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"directed-subscription-owner",
      L"owner",
      federationName));
  REQUIRE_NOTHROW(sender->joinFederationExecution(
      L"directed-subscription-sender",
      L"sender",
      federationName));
  REQUIRE_NOTHROW(ownershipSubscriber->joinFederationExecution(
      L"directed-subscription-by-owner",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(universalSubscriber->joinFederationExecution(
      L"directed-subscription-universal",
      L"subscriber",
      federationName));

  auto const objectClass = owner->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = owner->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const privilegeToDelete = owner->getAttributeHandle(
      objectClass,
      L"HLAprivilegeToDeleteObject");
  auto const interactionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  InteractionClassHandleSet const directedClasses{interactionClass};
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE(privilegeToDelete.isValid());

  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(sender->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(ownershipSubscriber->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(universalSubscriber->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(sender->publishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(owner->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(ownershipSubscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(universalSubscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      true));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = owner->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  drainCallbacks(*sender);
  drainCallbacks(*ownershipSubscriber);
  drainCallbacks(*universalSubscriber);
  REQUIRE(senderReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(ownershipSubscriberReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(universalSubscriberReports.objectDiscoveryReports.size() == 1U);

  // The default form is by ownership: the target owner receives this directed
  // interaction, the known non-owner does not, and universal receives it.
  REQUIRE_NOTHROW(sender->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  drainCallbacks(*owner);
  drainCallbacks(*ownershipSubscriber);
  drainCallbacks(*universalSubscriber);
  REQUIRE(ownerReports.directedInteractionReports.size() == 1U);
  REQUIRE(ownershipSubscriberReports.directedInteractionReports.empty());
  REQUIRE(universalSubscriberReports.directedInteractionReports.size() == 1U);
  REQUIRE(ownerReports.directedInteractionReports.front().objectInstance == target);
  REQUIRE(universalSubscriberReports.directedInteractionReports.front().objectInstance == target);
  REQUIRE(variableLengthDataBytes(
              universalSubscriberReports.directedInteractionReports.front().userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

  // An empty class set preserves the universal kind, irrespective of the
  // supplied boolean. The following send still reaches the universal target.
  REQUIRE_NOTHROW(universalSubscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      InteractionClassHandleSet{},
      false));
  REQUIRE_NOTHROW(sender->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  drainCallbacks(*owner);
  drainCallbacks(*ownershipSubscriber);
  drainCallbacks(*universalSubscriber);
  REQUIRE(ownerReports.directedInteractionReports.size() == 2U);
  REQUIRE(ownershipSubscriberReports.directedInteractionReports.empty());
  REQUIRE(universalSubscriberReports.directedInteractionReports.size() == 2U);

  // A by-ownership subscription is eligible when the receiver owns an
  // attribute on the addressed object. A second target keeps this path
  // independent from the universal target above.
  REQUIRE_NOTHROW(ownershipSubscriber->publishObjectClassAttributes(objectClass, {marker}));
  ObjectInstanceHandle ownershipTarget;
  REQUIRE_NOTHROW(ownershipTarget = ownershipSubscriber->registerObjectInstance(objectClass));
  drainCallbacks(*sender);
  drainCallbacks(*universalSubscriber);
  REQUIRE(ownershipSubscriber->isAttributeOwnedByFederate(ownershipTarget, marker));
  REQUIRE_FALSE(universalSubscriber->isAttributeOwnedByFederate(ownershipTarget, marker));
  REQUIRE_FALSE(universalSubscriber->isAttributeOwnedByFederate(
      ownershipTarget,
      privilegeToDelete));
  REQUIRE_NOTHROW(sender->sendDirectedInteraction(
      interactionClass,
      ownershipTarget,
      ParameterHandleValueMap{},
      tag));
  drainCallbacks(*owner);
  drainCallbacks(*ownershipSubscriber);
  drainCallbacks(*universalSubscriber);
  REQUIRE(ownerReports.directedInteractionReports.size() == 2U);
  REQUIRE(ownershipSubscriberReports.directedInteractionReports.size() == 1U);
  REQUIRE(ownershipSubscriberReports.directedInteractionReports.front().objectInstance ==
          ownershipTarget);
  REQUIRE(universalSubscriberReports.directedInteractionReports.size() == 3U);

  // Re-subscribing a supplied class changes only that class's mode.
  REQUIRE_NOTHROW(universalSubscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      false));
  REQUIRE_NOTHROW(sender->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  drainCallbacks(*owner);
  drainCallbacks(*ownershipSubscriber);
  drainCallbacks(*universalSubscriber);
  REQUIRE(ownerReports.directedInteractionReports.size() == 3U);
  REQUIRE(ownershipSubscriberReports.directedInteractionReports.size() == 1U);
  REQUIRE(universalSubscriberReports.directedInteractionReports.size() == 3U);

  REQUIRE_NOTHROW(ownershipSubscriber->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      true));
  REQUIRE_NOTHROW(sender->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag));
  drainCallbacks(*owner);
  drainCallbacks(*ownershipSubscriber);
  drainCallbacks(*universalSubscriber);
  REQUIRE(ownerReports.directedInteractionReports.size() == 4U);
  REQUIRE(ownershipSubscriberReports.directedInteractionReports.size() == 2U);
  REQUIRE(universalSubscriberReports.directedInteractionReports.size() == 3U);

  REQUIRE_NOTHROW(universalSubscriber->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(ownershipSubscriber->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(sender->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(universalSubscriber->disconnect());
  REQUIRE_NOTHROW(ownershipSubscriber->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
