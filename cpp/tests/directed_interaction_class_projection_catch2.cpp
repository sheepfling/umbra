#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The directed-interaction class projection test requires the Umbra source directory."
#endif

namespace {

namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::InteractionClassHandleSet;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

class ProjectionFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct Receipt final {
    InteractionClassHandle interactionClass;
    ObjectInstanceHandle objectInstance;
    FederateHandle producingFederate;
    std::vector<ParameterHandle> parameterHandles;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const&,
      FederateHandle const&) override {
    discoveries.emplace_back(objectInstance, objectClass);
  }

  void receiveDirectedInteraction(
      InteractionClassHandle const& interactionClass,
      ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      FederateHandle const& producingFederate) override {
    auto parameterHandles = std::vector<ParameterHandle>{};
    for (auto const& entry : parameterValues) {
      parameterHandles.push_back(entry.first);
    }
    directedReceipts.push_back({
        interactionClass,
        objectInstance,
        producingFederate,
        std::move(parameterHandles)});
  }

  std::vector<std::pair<ObjectInstanceHandle, ObjectClassHandle>> discoveries;
  std::vector<Receipt> directedReceipts;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path fixture(std::wstring_view name) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / std::filesystem::path(name);
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"directed-interaction-class-projection-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void drain(RTIambassador& ambassador) {
  for (int pass = 0; pass != 64; ++pass) {
    if (!ambassador.evokeCallback(0.0)) {
      break;
    }
  }
}

bool containsParameter(
    std::vector<ParameterHandle> const& parameterHandles,
    ParameterHandle const& parameterHandle) {
  return std::find(parameterHandles.begin(), parameterHandles.end(), parameterHandle) !=
      parameterHandles.end();
}

}  // namespace

TEST_CASE(
    "Embedded directed interaction reports closest subscribed class and parameters",
    "[integration][development-profile][interaction-management][directed]"
    "[received-class-projection][2025][rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.send-directed-interaction]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.receive-directed-interaction]") {
  ProjectionFederateAmbassador publisherCallbacks;
  ProjectionFederateAmbassador baseSubscriberCallbacks;
  ProjectionFederateAmbassador childSubscriberCallbacks;
  auto publisher = makeRti();
  auto baseSubscriber = makeRti();
  auto childSubscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const objectFom = fixture(L"directed-inherited-receipt-object-fom.xml").wstring();
  auto const interactionFom =
      fixture(L"directed-inherited-receipt-interaction-fom.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(baseSubscriber->connect(baseSubscriberCallbacks, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(childSubscriber->connect(childSubscriberCallbacks, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectFom, interactionFom},
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"projection-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(baseSubscriber->joinFederationExecution(
      L"projection-base-subscriber", L"base-subscriber", federationName));
  REQUIRE_NOTHROW(childSubscriber->joinFederationExecution(
      L"projection-child-subscriber", L"child-subscriber", federationName));

  auto const publisherTargetClass = publisher->getObjectClassHandle(
      fixture_hla::fom::directed_receipt_target_child);
  auto const baseTargetClass = baseSubscriber->getObjectClassHandle(
      fixture_hla::fom::directed_receipt_target_base);
  auto const childTargetClass = childSubscriber->getObjectClassHandle(
      fixture_hla::fom::directed_receipt_target_child);
  auto const publisherMarker = publisher->getAttributeHandle(
      publisherTargetClass, fixture_hla::fixture::target_marker);
  auto const baseMarker = baseSubscriber->getAttributeHandle(
      baseTargetClass, fixture_hla::fixture::target_marker);
  auto const childMarker = childSubscriber->getAttributeHandle(
      childTargetClass, fixture_hla::fixture::target_marker);
  auto const publisherChildInteraction = publisher->getInteractionClassHandle(
      fixture_hla::fom::directed_receipt_child);
  auto const baseInteraction = baseSubscriber->getInteractionClassHandle(
      fixture_hla::fom::directed_receipt_base);
  auto const childInteraction = childSubscriber->getInteractionClassHandle(
      fixture_hla::fom::directed_receipt_child);
  auto const publisherBaseParameter = publisher->getParameterHandle(
      publisherChildInteraction, fixture_hla::fixture::base_parameter);
  auto const publisherChildParameter = publisher->getParameterHandle(
      publisherChildInteraction, fixture_hla::fixture::child_parameter);
  auto const baseReceivedParameter = baseSubscriber->getParameterHandle(
      baseInteraction, fixture_hla::fixture::base_parameter);
  auto const childReceivedBaseParameter = childSubscriber->getParameterHandle(
      childInteraction, fixture_hla::fixture::base_parameter);
  auto const childReceivedChildParameter = childSubscriber->getParameterHandle(
      childInteraction, fixture_hla::fixture::child_parameter);
  REQUIRE(publisherTargetClass.isValid());
  REQUIRE(baseTargetClass.isValid());
  REQUIRE(childTargetClass.isValid());
  REQUIRE(publisherMarker.isValid());
  REQUIRE(baseMarker.isValid());
  REQUIRE(childMarker.isValid());
  REQUIRE(publisherChildInteraction.isValid());
  REQUIRE(baseInteraction.isValid());
  REQUIRE(childInteraction.isValid());
  REQUIRE(publisherBaseParameter.isValid());
  REQUIRE(publisherChildParameter.isValid());
  REQUIRE(baseReceivedParameter.isValid());
  REQUIRE(childReceivedBaseParameter.isValid());
  REQUIRE(childReceivedChildParameter.isValid());

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherTargetClass, rti1516_2025::AttributeHandleSet{publisherMarker}));
  REQUIRE_NOTHROW(baseSubscriber->subscribeObjectClassAttributes(
      baseTargetClass, rti1516_2025::AttributeHandleSet{baseMarker}));
  REQUIRE_NOTHROW(childSubscriber->subscribeObjectClassAttributes(
      childTargetClass, rti1516_2025::AttributeHandleSet{childMarker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      publisherTargetClass, InteractionClassHandleSet{publisherChildInteraction}));
  REQUIRE_NOTHROW(baseSubscriber->subscribeObjectClassDirectedInteractions(
      baseTargetClass, InteractionClassHandleSet{baseInteraction}, true));
  REQUIRE_NOTHROW(childSubscriber->subscribeObjectClassDirectedInteractions(
      childTargetClass, InteractionClassHandleSet{childInteraction}, true));

  auto const target = publisher->registerObjectInstance(publisherTargetClass);
  REQUIRE(target.isValid());
  drain(*baseSubscriber);
  drain(*childSubscriber);
  REQUIRE(baseSubscriberCallbacks.discoveries.size() == 1U);
  REQUIRE(baseSubscriberCallbacks.discoveries.front().first == target);
  REQUIRE(baseSubscriberCallbacks.discoveries.front().second == baseTargetClass);
  REQUIRE(childSubscriberCallbacks.discoveries.size() == 1U);
  REQUIRE(childSubscriberCallbacks.discoveries.front().first == target);
  REQUIRE(childSubscriberCallbacks.discoveries.front().second == childTargetClass);

  std::array<std::uint8_t, 1U> const baseValue{0x42U};
  std::array<std::uint8_t, 1U> const childValue{0x43U};
  ParameterHandleValueMap parameterValues;
  parameterValues.emplace(
      publisherBaseParameter, VariableLengthData(baseValue.data(), baseValue.size()));
  parameterValues.emplace(
      publisherChildParameter, VariableLengthData(childValue.data(), childValue.size()));
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      publisherChildInteraction, target, parameterValues, VariableLengthData{}));
  drain(*baseSubscriber);
  drain(*childSubscriber);

  REQUIRE(baseSubscriberCallbacks.directedReceipts.size() == 1U);
  auto const& baseReceipt = baseSubscriberCallbacks.directedReceipts.front();
  REQUIRE(baseReceipt.interactionClass == baseInteraction);
  REQUIRE(baseReceipt.objectInstance == target);
  REQUIRE(baseReceipt.producingFederate.isValid());
  REQUIRE(baseReceipt.parameterHandles.size() == 1U);
  REQUIRE(containsParameter(baseReceipt.parameterHandles, baseReceivedParameter));

  REQUIRE(childSubscriberCallbacks.directedReceipts.size() == 1U);
  auto const& childReceipt = childSubscriberCallbacks.directedReceipts.front();
  REQUIRE(childReceipt.interactionClass == childInteraction);
  REQUIRE(childReceipt.objectInstance == target);
  REQUIRE(childReceipt.producingFederate.isValid());
  REQUIRE(childReceipt.parameterHandles.size() == 2U);
  REQUIRE(containsParameter(childReceipt.parameterHandles, childReceivedBaseParameter));
  REQUIRE(containsParameter(childReceipt.parameterHandles, childReceivedChildParameter));

  REQUIRE_NOTHROW(childSubscriber->unsubscribeObjectClassDirectedInteractions(childTargetClass));
  REQUIRE_NOTHROW(baseSubscriber->unsubscribeObjectClassDirectedInteractions(baseTargetClass));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassDirectedInteractions(publisherTargetClass));
  REQUIRE_NOTHROW(childSubscriber->unsubscribeObjectClassAttributes(
      childTargetClass, rti1516_2025::AttributeHandleSet{childMarker}));
  REQUIRE_NOTHROW(baseSubscriber->unsubscribeObjectClassAttributes(
      baseTargetClass, rti1516_2025::AttributeHandleSet{baseMarker}));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      publisherTargetClass, rti1516_2025::AttributeHandleSet{publisherMarker}));
  REQUIRE_NOTHROW(childSubscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(baseSubscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(childSubscriber->disconnect());
  REQUIRE_NOTHROW(baseSubscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
