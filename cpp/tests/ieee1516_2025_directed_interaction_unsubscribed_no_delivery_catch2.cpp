#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The directed interaction no-delivery test requires the Umbra source directory."
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

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"directed-interaction-unsubscribed-no-delivery-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path testDataPath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

class DeliveryReports final : public rti1516_2025::NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const&,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    ++discoveries;
  }

  void receiveDirectedInteraction(
      InteractionClassHandle const&,
      ObjectInstanceHandle const&,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&) override {
    ++directedInteractions;
  }

  std::size_t discoveries = 0U;
  std::size_t directedInteractions = 0U;
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
    "Embedded directed interaction is not delivered to a known target without a class subscription",
    "[integration][development-profile][2025][interaction-management][directed]"
    "[directed-interaction-no-applicable-subscription-2025]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.send-directed-interaction]"
    "[federate.callback.receive-directed-interaction]") {
  DeliveryReports publisherReports;
  DeliveryReports receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer =
      testDataPath("directed-interaction-object-consumer-fom.xml").wstring();
  auto const interactionProvider =
      testDataPath("directed-interaction-interaction-provider-fom.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"directed-nondelivery-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"directed-nondelivery-known-target",
      L"receiver",
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

  InteractionClassHandleSet const directedClasses{interactionClass};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  // The receiver intentionally has no Subscribe Object Class Directed
  // Interactions declaration for this class.

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.discoveries == 1U);
  REQUIRE(receiverReports.directedInteractions == 0U);

  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      VariableLengthData{}));
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.directedInteractions == 0U);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
