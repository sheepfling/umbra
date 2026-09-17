#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped directed-interaction re-enable test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::InteractionClassHandleSet;
using rti1516_2025::LogicalTime;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RECEIVE;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
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
  return L"timestamped-directed-interaction-reenable-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
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
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
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
      FederateHandle const& producingFederate,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    directedInteractionReports.push_back({
        interactionClass,
        objectInstance,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("directed");
  }

  void timeConstrainedEnabled(LogicalTime const&) override {
    ++timeConstrainedEnabledCount;
  }

  void timeAdvanceGrant(LogicalTime const&) override {
    ++timeAdvanceGrantCount;
    callbackOrder.push_back("grant");
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<DirectedInteractionReport> directedInteractionReports;
  std::size_t timeConstrainedEnabledCount = 0U;
  std::size_t timeAdvanceGrantCount = 0U;
  std::vector<std::string> callbackOrder;
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
    "Embedded timestamped directed interaction survives time-constrained re-enable",
    "[integration][development-profile][interaction-management][directed][time-management]"
    "[timestamped-directed-interaction][tso][re-enable]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.send-directed-interaction]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.disable-time-constrained][rti.service.time-advance-request]"
    "[federate.callback.receive-directed-interaction]"
    "[federate.callback.time-constrained-enabled]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = resourcePath(
      "directed-interaction-object-consumer-fom.xml").wstring();
  auto const interactionProvider = resourcePath(
      "directed-interaction-interaction-provider-fom.xml").wstring();
  std::vector<unsigned char> const tagBytes{
      0x52U, 0x45U, 0x45U, 0x4EU, 0x2DU, 0x44U};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  InteractionClassHandleSet directedClasses;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-directed-reenable-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-directed-reenable-receiver",
      L"subscriber",
      federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const receiverObjectClass = receiver->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const marker = publisher->getAttributeHandle(
      objectClass,
      L"DirectedTargetMarker");
  auto const receiverMarker = receiver->getAttributeHandle(
      receiverObjectClass,
      L"DirectedTargetMarker");
  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(objectClass.isValid());
  REQUIRE(receiverObjectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(receiverMarker.isValid());
  REQUIRE(interactionClass.isValid());
  directedClasses.insert(interactionClass);

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      objectClass,
      rti1516_2025::AttributeHandleSet{marker}));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      receiverObjectClass,
      rti1516_2025::AttributeHandleSet{receiverMarker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(
      interactionClass,
      TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
      receiverObjectClass,
      directedClasses,
      true));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeConstrainedEnabledCount == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);

  auto const retraction = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.directedInteractionReports.empty());

  // A queued directed TSO passel is tied to the joined-federate lifetime, not
  // to one particular Time Constrained enablement.
  REQUIRE_NOTHROW(receiver->disableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.directedInteractionReports.empty());
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeConstrainedEnabledCount == 2U);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.directedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantCount == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"directed", "grant"});

  auto const& report = receiverReports.directedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.objectInstance == target);
  REQUIRE(report.parameterValues.empty());
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) == tagBytes);
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.transportationType ==
          publisher->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == TIMESTAMP);
  REQUIRE(report.receivedOrderType == TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassDirectedInteractions(
      receiverObjectClass,
      directedClasses));
  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(
      receiverObjectClass,
      rti1516_2025::AttributeHandleSet{receiverMarker}));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
