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
#error "The timestamped directed-interaction source-resignation test requires the Umbra source directory."
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
  return L"timestamped-directed-interaction-source-resignation-" +
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

  void timeRegulationEnabled(LogicalTime const&) override {
    ++timeRegulationEnabledCount;
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    ++timeAdvanceGrantCount;
    timeAdvanceGrantValue = time.toString();
    callbackOrder.push_back("grant");
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<DirectedInteractionReport> directedInteractionReports;
  std::size_t timeConstrainedEnabledCount = 0U;
  std::size_t timeRegulationEnabledCount = 0U;
  std::size_t timeAdvanceGrantCount = 0U;
  std::wstring timeAdvanceGrantValue;
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
    "Embedded queued timestamped directed interaction survives source resignation",
    "[integration][development-profile][interaction-management][directed][time-management]"
    "[timestamped-directed-interaction][tso][resignation]"
    "[rti.service.send-directed-interaction][rti.service.resign-federation-execution]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request][federate.callback.receive-directed-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador senderReports;
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador clockReports;
  auto owner = makeRti();
  auto sender = makeRti();
  auto receiver = makeRti();
  auto clock = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = resourcePath(
      "directed-interaction-object-consumer-fom.xml").wstring();
  auto const interactionProvider = resourcePath(
      "directed-interaction-interaction-provider-fom.xml").wstring();
  std::vector<unsigned char> const tagBytes{
      0x52U, 0x45U, 0x53U, 0x49U, 0x47U};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(clock->connect(clockReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"timestamped-directed-resignation-owner",
      L"owner",
      federationName));
  FederateHandle senderHandle;
  REQUIRE_NOTHROW(senderHandle = sender->joinFederationExecution(
      L"timestamped-directed-resignation-sender",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-directed-resignation-receiver",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(clock->joinFederationExecution(
      L"timestamped-directed-resignation-clock",
      L"publisher",
      federationName));

  auto const objectClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const ownerMarker = owner->getAttributeHandle(
      objectClass,
      L"DirectedTargetMarker");
  auto const senderObjectClass = sender->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const senderMarker = sender->getAttributeHandle(
      senderObjectClass,
      L"DirectedTargetMarker");
  auto const receiverObjectClass = receiver->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const receiverMarker = receiver->getAttributeHandle(
      receiverObjectClass,
      L"DirectedTargetMarker");
  auto const interactionClass = owner->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(objectClass.isValid());
  REQUIRE(ownerMarker.isValid());
  REQUIRE(senderMarker.isValid());
  REQUIRE(receiverMarker.isValid());
  REQUIRE(interactionClass.isValid());
  InteractionClassHandleSet const directedClassSet{interactionClass};

  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      objectClass,
      rti1516_2025::AttributeHandleSet{ownerMarker}));
  REQUIRE_NOTHROW(sender->subscribeObjectClassAttributes(
      senderObjectClass,
      rti1516_2025::AttributeHandleSet{senderMarker}));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      receiverObjectClass,
      rti1516_2025::AttributeHandleSet{receiverMarker}));
  REQUIRE_NOTHROW(sender->publishObjectClassDirectedInteractions(
      senderObjectClass,
      directedClassSet));
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(
      interactionClass,
      TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
      receiverObjectClass,
      directedClassSet,
      true));

  ObjectInstanceHandle registeredTarget;
  REQUIRE_NOTHROW(registeredTarget = owner->registerObjectInstance(objectClass));
  REQUIRE(registeredTarget.isValid());
  drainCallbacks(*sender);
  drainCallbacks(*receiver);
  REQUIRE(senderReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  auto const target = senderReports.objectDiscoveryReports.front();
  REQUIRE(target == registeredTarget);
  REQUIRE(receiverReports.objectDiscoveryReports.front() == target);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeConstrainedEnabledCount == 1U);
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*sender);
  REQUIRE(senderReports.timeRegulationEnabledCount == 1U);
  REQUIRE_NOTHROW(clock->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*clock);
  REQUIRE(clockReports.timeRegulationEnabledCount == 1U);
  auto const reliableTransportation =
      sender->getTransportationTypeHandle(standard_hla::mom::reliable);

  auto const retraction = sender->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.directedInteractionReports.empty());

  // The target belongs to a surviving owner, while the interaction source
  // resigns voluntarily. The accepted TSO recipient must outlive that source
  // membership and retain its target projection until the grant boundary.
  REQUIRE_NOTHROW(sender->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE(receiverReports.directedInteractionReports.empty());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(clock->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*clock);
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
  REQUIRE(report.transportationType == reliableTransportation);
  REQUIRE(report.producingFederate == senderHandle);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == TIMESTAMP);
  REQUIRE(report.receivedOrderType == TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(receiverReports.timeAdvanceGrantValue == L"6");

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(clock->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(clock->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
