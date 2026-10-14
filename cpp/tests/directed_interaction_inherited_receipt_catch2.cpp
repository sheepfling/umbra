#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

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
#error "The directed-interaction receipt test requires the Umbra source directory."
#endif

namespace {

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
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

class ReceiptFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct DirectedReceipt final {
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
    auto receivedParameterHandles = std::vector<ParameterHandle>{};
    for (auto const& entry : parameterValues) {
      receivedParameterHandles.push_back(entry.first);
    }
    directedReceipts.push_back(
        {interactionClass, objectInstance, producingFederate, std::move(receivedParameterHandles)});
  }

  void receiveDirectedInteraction(
      InteractionClassHandle const& interactionClass,
      ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      FederateHandle const& producingFederate,
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::OrderType,
      rti1516_2025::OrderType,
      rti1516_2025::MessageRetractionHandle const*) override {
    auto receivedParameterHandles = std::vector<ParameterHandle>{};
    for (auto const& entry : parameterValues) {
      receivedParameterHandles.push_back(entry.first);
    }
    timestampedDirectedReceipts.push_back({
        interactionClass,
        objectInstance,
        producingFederate,
        std::move(receivedParameterHandles)});
    timestampedReceiptTime = dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)
        ? static_cast<rti1516_2025::HLAinteger64Time const&>(time).getTime()
        : -1;
  }

  std::vector<std::pair<ObjectInstanceHandle, ObjectClassHandle>> discoveries;
  std::vector<DirectedReceipt> directedReceipts;
  std::vector<DirectedReceipt> timestampedDirectedReceipts;
  std::int64_t timestampedReceiptTime = -1;
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
  return L"directed-interaction-inherited-receipt-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void drain(RTIambassador& ambassador) {
  for (int pass = 0; pass != 64; ++pass) {
    if (!ambassador.evokeCallback(0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Embedded directed receipt selects an ancestor subscription at the known target class",
    "[integration][development-profile][interaction-management][directed]"
    "[interaction-receipt-inheritance][timestamped-directed-interaction][2025]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.register-object-instance][rti.service.send-directed-interaction]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.receive-directed-interaction]") {
  ReceiptFederateAmbassador publisherCallbacks;
  ReceiptFederateAmbassador receiverCallbacks;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const objectFom = fixture(L"directed-inherited-receipt-object-fom.xml").wstring();
  auto const interactionFom =
      fixture(L"directed-inherited-receipt-interaction-fom.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverCallbacks, HLA_EVOKED));
  try {
    publisher->createFederationExecution(
        federationName,
        std::vector<std::wstring>{objectFom, interactionFom},
        L"HLAinteger64Time");
  } catch (rti1516_2025::Exception const& error) {
    auto const name = error.name();
    auto const message = error.what();
    FAIL("Directed-receipt FOM creation failed with RTI exception "
         << std::string(name.begin(), name.end()) << ": "
         << std::string(message.begin(), message.end()));
  }
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"directed-receipt-publisher", L"publisher", federationName));
  try {
    receiver->joinFederationExecution(
        L"directed-receipt-subscriber", L"subscriber", federationName);
  } catch (rti1516_2025::Exception const& error) {
    auto const name = error.name();
    auto const message = error.what();
    FAIL("Directed-receipt receiver join failed with RTI exception "
         << std::string(name.begin(), name.end()) << ": "
         << std::string(message.begin(), message.end()));
  }

  auto const publisherTargetClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDirectedReceiptTargetBase.UmbraDirectedReceiptTargetChild");
  auto const receiverKnownTargetClass = receiver->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDirectedReceiptTargetBase");
  auto const receiverRegisteredTargetClass = receiver->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDirectedReceiptTargetBase.UmbraDirectedReceiptTargetChild");
  auto const publisherMarker = publisher->getAttributeHandle(
      publisherTargetClass, L"TargetMarker");
  auto const receiverMarker = receiver->getAttributeHandle(
      receiverKnownTargetClass, L"TargetMarker");
  auto const publisherChildInteraction = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDirectedReceiptBase.UmbraDirectedReceiptChild");
  auto const receiverBaseInteraction = receiver->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDirectedReceiptBase");
  auto const receiverChildInteraction = receiver->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDirectedReceiptBase.UmbraDirectedReceiptChild");
  auto const publisherBaseParameter = publisher->getParameterHandle(
      publisherChildInteraction, L"BaseParameter");
  auto const publisherChildParameter = publisher->getParameterHandle(
      publisherChildInteraction, L"ChildParameter");
  auto const receiverBaseParameter = receiver->getParameterHandle(
      receiverBaseInteraction, L"BaseParameter");
  REQUIRE(publisherTargetClass.isValid());
  REQUIRE(receiverKnownTargetClass.isValid());
  REQUIRE(receiverRegisteredTargetClass.isValid());
  REQUIRE(publisherMarker.isValid());
  REQUIRE(receiverMarker.isValid());
  REQUIRE(publisherChildInteraction.isValid());
  REQUIRE(receiverBaseInteraction.isValid());
  REQUIRE(receiverChildInteraction.isValid());
  REQUIRE(publisherBaseParameter.isValid());
  REQUIRE(publisherChildParameter.isValid());
  REQUIRE(receiverBaseParameter.isValid());

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherTargetClass, rti1516_2025::AttributeHandleSet{publisherMarker}));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      receiverKnownTargetClass, rti1516_2025::AttributeHandleSet{receiverMarker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      publisherTargetClass, InteractionClassHandleSet{publisherChildInteraction}));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
      receiverKnownTargetClass,
      InteractionClassHandleSet{receiverBaseInteraction},
      true));
  // This deeper subscription must not apply because the target is only known
  // to this receiver at the base object class.
  REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
      receiverRegisteredTargetClass,
      InteractionClassHandleSet{receiverChildInteraction},
      true));

  auto const target = publisher->registerObjectInstance(publisherTargetClass);
  REQUIRE(target.isValid());
  drain(*receiver);
  REQUIRE(receiverCallbacks.discoveries.size() == 1U);
  REQUIRE(receiverCallbacks.discoveries.front().first == target);
  REQUIRE(receiverCallbacks.discoveries.front().second == receiverKnownTargetClass);

  std::array<std::uint8_t, 1U> const baseParameterValue{0x42U};
  std::array<std::uint8_t, 1U> const childParameterValue{0x43U};
  ParameterHandleValueMap sentParameterValues;
  sentParameterValues.emplace(
      publisherBaseParameter,
      VariableLengthData(baseParameterValue.data(), baseParameterValue.size()));
  sentParameterValues.emplace(
      publisherChildParameter,
      VariableLengthData(childParameterValue.data(), childParameterValue.size()));
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      publisherChildInteraction,
      target,
      sentParameterValues,
      VariableLengthData{}));
  drain(*receiver);
  REQUIRE(receiverCallbacks.directedReceipts.size() == 1U);
  REQUIRE(receiverCallbacks.directedReceipts.front().interactionClass == receiverBaseInteraction);
  REQUIRE(receiverCallbacks.directedReceipts.front().objectInstance == target);
  REQUIRE(receiverCallbacks.directedReceipts.front().producingFederate.isValid());
  REQUIRE(receiverCallbacks.directedReceipts.front().parameterHandles ==
          std::vector<ParameterHandle>{receiverBaseParameter});

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(0)));
  drain(*publisher);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drain(*receiver);
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  drain(*receiver);
  std::array<std::uint8_t, 3U> const timestampedTagBytes{0x44U, 0x54U, 0x53U};
  REQUIRE_NOTHROW(publisher->sendDirectedInteraction(
      publisherChildInteraction,
      target,
      sentParameterValues,
      VariableLengthData(timestampedTagBytes.data(), timestampedTagBytes.size()),
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  drain(*publisher);
  drain(*receiver);
  REQUIRE(receiverCallbacks.timestampedDirectedReceipts.size() == 1U);
  REQUIRE(receiverCallbacks.timestampedDirectedReceipts.front().interactionClass ==
          receiverBaseInteraction);
  REQUIRE(receiverCallbacks.timestampedDirectedReceipts.front().objectInstance == target);
  REQUIRE(receiverCallbacks.timestampedDirectedReceipts.front().producingFederate.isValid());
  REQUIRE(receiverCallbacks.timestampedDirectedReceipts.front().parameterHandles ==
          std::vector<ParameterHandle>{receiverBaseParameter});
  REQUIRE(receiverCallbacks.timestampedReceiptTime == 5);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassDirectedInteractions(
      receiverKnownTargetClass));
  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassDirectedInteractions(
      receiverRegisteredTargetClass));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassDirectedInteractions(
      publisherTargetClass));
  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(
      receiverKnownTargetClass, rti1516_2025::AttributeHandleSet{receiverMarker}));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      publisherTargetClass, rti1516_2025::AttributeHandleSet{publisherMarker}));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
