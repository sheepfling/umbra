#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

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
#error "The timestamped directed resignation test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::InteractionClassHandleSet;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RECEIVE;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"timestamped-directed-resignation-multi-recipient-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr || value.size() == 0U) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

class ReportingFederateAmbassador final : public NullFederateAmbassador {
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

  struct TimeAdvanceGrantReport final {
    std::wstring implementationName;
    std::wstring value;
  };

  struct FlushQueueGrantReport final {
    std::wstring implementationName;
    std::wstring value;
    std::wstring optimisticValue;
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
      MessageRetractionHandle const* optionalRetraction) override {
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

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  void flushQueueGrant(
      LogicalTime const& time,
      LogicalTime const& optimisticTime) override {
    flushQueueGrantReports.push_back({
        time.implementationName(),
        time.toString(),
        optimisticTime.toString(),
    });
    callbackOrder.push_back("flush-grant");
  }

  void timeRegulationEnabled(LogicalTime const& time) override {
    timeRegulationEnabledReports.push_back({time.implementationName(), time.toString()});
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<DirectedInteractionReport> directedInteractionReports;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<FlushQueueGrantReport> flushQueueGrantReports;
  std::vector<TimeAdvanceGrantReport> timeRegulationEnabledReports;
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
    "Embedded queued timestamped directed interaction survives source resignation for each recipient",
    "[integration][development-profile][federation-management][interaction-management]"
    "[directed][time-management][timestamped-directed-interaction][tso]"
    "[resignation][multi-federate-callback-ordering]"
    "[timestamped-directed-interaction-multi-recipient-source-resignation]"
    "[rti.service.send-directed-interaction][rti.service.resign-federation-execution]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request][rti.service.flush-queue-request]"
    "[rti.service.time-advance-request-available][rti.service.next-message-request-available]"
    "[federate.callback.receive-directed-interaction]"
    "[federate.callback.time-advance-grant][federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador fqrReports;
  ReportingFederateAmbassador taraReports;
  ReportingFederateAmbassador nmraReports;
  auto owner = makeRti();
  auto publisher = makeRti();
  auto fqr = makeRti();
  auto tara = makeRti();
  auto nmra = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = resourcePath(
      "directed-interaction-object-consumer-fom.xml").wstring();
  auto const interactionProvider = resourcePath(
      "directed-interaction-interaction-provider-fom.xml").wstring();
  unsigned char const tagBytes[] = {0x52, 0x45, 0x53, 0x49, 0x47};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(fqr->connect(fqrReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tara->connect(taraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmra->connect(nmraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"timestamped-directed-resignation-owner",
      L"owner",
      federationName));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-directed-resignation-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(fqr->joinFederationExecution(
      L"timestamped-directed-resignation-fqr",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(tara->joinFederationExecution(
      L"timestamped-directed-resignation-tara",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(nmra->joinFederationExecution(
      L"timestamped-directed-resignation-nmra",
      L"subscriber",
      federationName));

  auto const objectClass = owner->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = owner->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  auto const publisherObjectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const publisherMarker = publisher->getAttributeHandle(
      publisherObjectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const publisherInteractionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE(publisherObjectClass.isValid());
  REQUIRE(publisherMarker.isValid());
  REQUIRE(publisherInteractionClass.isValid());
  InteractionClassHandleSet const directedClasses{interactionClass};

  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(publisher->subscribeObjectClassAttributes(
      publisherObjectClass,
      {publisherMarker}));
  for (auto* recipient : {fqr.get(), tara.get(), nmra.get()}) {
    REQUIRE_NOTHROW(recipient->subscribeObjectClassAttributes(objectClass, {marker}));
  }
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      publisherObjectClass,
      InteractionClassHandleSet{publisherInteractionClass}));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(
      publisherInteractionClass,
      TIMESTAMP));
  for (auto* recipient : {fqr.get(), tara.get(), nmra.get()}) {
    REQUIRE_NOTHROW(recipient->subscribeObjectClassDirectedInteractions(
        objectClass,
        directedClasses,
        true));
  }

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = owner->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  for (auto* recipient : {fqr.get(), tara.get(), nmra.get()}) {
    drainCallbacks(*recipient);
  }
  drainCallbacks(*publisher);
  REQUIRE(fqrReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(taraReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(nmraReports.objectDiscoveryReports.size() == 1U);

  for (auto* recipient : {fqr.get(), tara.get(), nmra.get()}) {
    REQUIRE_NOTHROW(recipient->enableTimeConstrained());
    drainCallbacks(*recipient);
  }
  // Keep an independent regulator in the execution after the directed
  // producer resigns.  Its lookahead is deliberately beyond the queued
  // message so TAR/NMR-Available can each cross the same TSO boundary.
  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(10)));
  drainCallbacks(*owner);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);
  auto const retraction = publisher->sendDirectedInteraction(
      publisherInteractionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(fqrReports.directedInteractionReports.empty());
  REQUIRE(taraReports.directedInteractionReports.empty());
  REQUIRE(nmraReports.directedInteractionReports.empty());

  // The target remains owned by the surviving federate. The pending TSO
  // delivery must retain the departed producer identity and be released
  // independently for each recipient.
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE(fqrReports.directedInteractionReports.empty());
  REQUIRE(taraReports.directedInteractionReports.empty());
  REQUIRE(nmraReports.directedInteractionReports.empty());

  REQUIRE_NOTHROW(fqr->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(tara->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(nmra->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  while (ownerReports.timeAdvanceGrantReports.empty() &&
         owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(ownerReports.timeAdvanceGrantReports.front().value == L"2");

  for (auto* recipient : {fqr.get(), tara.get(), nmra.get()}) {
    drainCallbacks(*recipient);
  }
  REQUIRE(fqrReports.directedInteractionReports.size() == 1U);
  REQUIRE(fqrReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(fqrReports.flushQueueGrantReports.front().value == L"7");
  REQUIRE(fqrReports.flushQueueGrantReports.front().optimisticValue == L"7");
  REQUIRE(fqrReports.callbackOrder == std::vector<std::string>{"directed", "flush-grant"});
  REQUIRE(taraReports.directedInteractionReports.size() == 1U);
  REQUIRE(taraReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(taraReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(taraReports.callbackOrder == std::vector<std::string>{"directed", "grant"});
  REQUIRE(nmraReports.directedInteractionReports.size() == 1U);
  REQUIRE(nmraReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(nmraReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(nmraReports.callbackOrder == std::vector<std::string>{"directed", "grant"});

  auto const deliveredTransportation =
      fqrReports.directedInteractionReports.front().transportationType;
  REQUIRE(deliveredTransportation.isValid());
  auto requireDirected = [&](auto const& report) {
    REQUIRE(report.interactionClass == interactionClass);
    REQUIRE(report.objectInstance == target);
    REQUIRE(report.parameterValues.empty());
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.transportationType == deliveredTransportation);
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == TIMESTAMP);
    REQUIRE(report.receivedOrderType == TIMESTAMP);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  };
  requireDirected(fqrReports.directedInteractionReports.front());
  requireDirected(taraReports.directedInteractionReports.front());
  requireDirected(nmraReports.directedInteractionReports.front());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(fqr->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(tara->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(nmra->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(fqr->disconnect());
  REQUIRE_NOTHROW(tara->disconnect());
  REQUIRE_NOTHROW(nmra->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
