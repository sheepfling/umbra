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
#error "The Flush Queue optimistic-time test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandle;
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
  return L"flush-queue-request-optimistic-time-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path sourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct TimestampedInteractionReport final {
    InteractionClassHandle interactionClass;
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

  struct ReceiveOrderInteractionReport final {
    InteractionClassHandle interactionClass;
    FederateHandle producingFederate;
  };

  struct FlushQueueGrantReport final {
    std::wstring timeImplementationName;
    std::wstring value;
    std::wstring optimisticValue;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*) override {
    receiveOrderInteractionReports.push_back({interactionClass, producingFederate});
    callbackOrder.push_back("receive-order-interaction");
  }

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle const* optionalRetraction) override {
    timestampedInteractionReports.push_back({
        interactionClass,
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
    callbackOrder.push_back("interaction");
  }

  void flushQueueGrant(
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::LogicalTime const& optimisticTime) override {
    flushQueueGrantReports.push_back({
        time.implementationName(),
        time.toString(),
        optimisticTime.toString(),
    });
    callbackOrder.push_back("flush-grant");
  }

  std::vector<TimestampedInteractionReport> timestampedInteractionReports;
  std::vector<ReceiveOrderInteractionReport> receiveOrderInteractionReports;
  std::vector<FlushQueueGrantReport> flushQueueGrantReports;
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
    "Embedded Flush Queue Request flushes queued TSO and reports optimistic time",
    "[integration][development-profile][time-management]"
    "[flush-queue-request][flush-queue-request-optimistic-time][tso]"
    "[galt][optimistic-time][callbacks][fqr-olt-minimum-delivered-tso]"
    "[rti.service.flush-queue-request][rti.service.send-interaction]"
    "[federate.callback.receive-interaction][federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "parameter-handle-provider-fom.xml").wstring();
  unsigned char const identifierBytes[] = {0xF4U, 0x54U};
  unsigned char const tagBytes[] = {0x46U, 0x51U, 0x52U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ParameterHandleValueMap parameterValues;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"flush-queue-optimistic-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"flush-queue-optimistic-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(
      interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);

  REQUIRE_NOTHROW(publisher->sendInteraction(
      interactionClass, parameterValues, tag));
  REQUIRE(receiverReports.receiveOrderInteractionReports.empty());

  auto const first = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  auto const second = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(12));
  REQUIRE(first.isValid());
  REQUIRE(second.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE(receiverReports.flushQueueGrantReports.empty());

  // FQR delivers the queued RO message and every queued TSO payload, including
  // the one beyond the requested time. All message callbacks precede the grant.
  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.receiveOrderInteractionReports.size() == 1U);
  REQUIRE(receiverReports.receiveOrderInteractionReports.front().interactionClass == interactionClass);
  REQUIRE(receiverReports.receiveOrderInteractionReports.front().producingFederate == publisherHandle);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2U);
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 1U);
  auto const& grant = receiverReports.flushQueueGrantReports.front();
  REQUIRE(grant.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(grant.value == L"5");
  REQUIRE(grant.optimisticValue == L"7");
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"receive-order-interaction", "interaction", "interaction", "flush-grant"});

  for (std::size_t index = 0U;
       index < receiverReports.timestampedInteractionReports.size();
       ++index) {
    auto const& report = receiverReports.timestampedInteractionReports[index];
    REQUIRE(report.interactionClass == interactionClass);
    REQUIRE(report.parameterValues.size() == 1U);
    REQUIRE(report.parameterValues.contains(identifier));
    REQUIRE(variableLengthDataBytes(report.parameterValues.at(identifier)) ==
            std::vector<unsigned char>(
                identifierBytes, identifierBytes + sizeof(identifierBytes)));
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.transportationType.isValid());
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == (index == 0U ? L"7" : L"12"));
    REQUIRE(report.sentOrderType == TIMESTAMP);
    REQUIRE(report.receivedOrderType == TIMESTAMP);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  }

  rti1516_2025::HLAinteger64Time receiverTime;
  REQUIRE_NOTHROW(receiver->queryLogicalTime(receiverTime));
  REQUIRE(receiverTime.getTime() == 5);
  REQUIRE_THROWS_AS(
      receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::LogicalTimeAlreadyPassed);

  // The publisher's new lower bound is 20, above this request. With no queued
  // TSO deciding the result, FQR's actual and optimistic times must agree.
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(15)));
  drainCallbacks(*publisher);
  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 2U);
  REQUIRE(receiverReports.flushQueueGrantReports.back().value == L"10");
  REQUIRE(receiverReports.flushQueueGrantReports.back().optimisticValue == L"10");
  REQUIRE_NOTHROW(receiver->queryLogicalTime(receiverTime));
  REQUIRE(receiverTime.getTime() == 10);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded FQG applies deferred lookahead after the granted logical-time advance",
    "[integration][development-profile][2025][time-management]"
    "[flush-queue-request][modify-lookahead][deferred-lookahead]"
    "[federate.callback.flush-queue-grant][rti.service.query-lookahead]") {
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador peerReports;
  auto requester = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "parameter-handle-provider-fom.xml").wstring();

  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"fqr-deferred-lookahead-requester", L"time-regulating", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"fqr-deferred-lookahead-peer", L"time-regulating", federationName));

  REQUIRE_NOTHROW(requester->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(10)));
  drainCallbacks(*requester);
  REQUIRE_NOTHROW(peer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*peer);

  REQUIRE_NOTHROW(requester->modifyLookahead(
      rti1516_2025::HLAinteger64Interval(2)));
  rti1516_2025::HLAinteger64Interval lookahead;
  REQUIRE_NOTHROW(requester->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 10);

  // The peer's GALT bounds FQG at 5. Advancing from 0 to 5 reduces the old
  // lookahead 10 by 5; max(requested 2, remaining 5) is the new actual value.
  REQUIRE_NOTHROW(requester->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(requesterReports.flushQueueGrantReports.front().value == L"5");
  REQUIRE(requesterReports.flushQueueGrantReports.front().optimisticValue == L"10");
  REQUIRE_NOTHROW(requester->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);

  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded early FQR grant eases the subsequent TSO timestamp bound",
    "[integration][development-profile][2025][time-management][tso]"
    "[fqr-early-grant-tso-bound-easing][rti.service.flush-queue-request]"
    "[rti.service.send-interaction][rti.service.query-logical-time]"
    "[federate.callback.receive-interaction][federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador regulatingReports;
  ReportingFederateAmbassador peerReports;
  auto regulating = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "parameter-handle-provider-fom.xml").wstring();
  unsigned char const valueBytes[] = {0x46U, 0x51U, 0x52U};
  unsigned char const tagBytes[] = {0x45U, 0x41U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(regulating->connect(regulatingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulating->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(regulating->joinFederationExecution(
      L"fqr-early-grant-regulating", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"fqr-early-grant-peer", L"publisher", federationName));

  auto const interactionClass = regulating->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = regulating->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE_NOTHROW(regulating->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(peer->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(regulating->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(peer->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(regulating->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(regulating->enableTimeConstrained());
  drainCallbacks(*regulating);
  REQUIRE_NOTHROW(regulating->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*regulating);
  REQUIRE_NOTHROW(peer->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*peer);

  ParameterHandleValueMap incomingValues;
  incomingValues.emplace(identifier, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const incoming = peer->sendInteraction(
      interactionClass, incomingValues, tag, rti1516_2025::HLAinteger64Time(1));
  REQUIRE(incoming.isValid());
  REQUIRE(regulatingReports.timestampedInteractionReports.empty());

  // Both regulators are at logical time 0 with lookahead 1, so the TSO at 1
  // is exactly the GALT frontier. FQR(10) grants at 1; after leaving Time
  // Advancing, the sender bound is current time 1 plus lookahead 1.
  REQUIRE_NOTHROW(regulating->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(regulatingReports.timestampedInteractionReports.empty());
  REQUIRE(regulatingReports.flushQueueGrantReports.empty());
  drainCallbacks(*regulating);
  REQUIRE(regulatingReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(regulatingReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(regulatingReports.timestampedInteractionReports.front().timeValue == L"1");
  REQUIRE(regulatingReports.flushQueueGrantReports.front().value == L"1");
  REQUIRE(regulatingReports.flushQueueGrantReports.front().optimisticValue == L"1");
  REQUIRE(regulatingReports.callbackOrder ==
          std::vector<std::string>{"interaction", "flush-grant"});

  rti1516_2025::HLAinteger64Time logicalTime;
  REQUIRE_NOTHROW(regulating->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 1);
  ParameterHandleValueMap outgoingValues;
  outgoingValues.emplace(identifier, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const outgoing = regulating->sendInteraction(
      interactionClass, outgoingValues, tag, rti1516_2025::HLAinteger64Time(2));
  REQUIRE(outgoing.isValid());
  REQUIRE_NOTHROW(regulating->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 1);

  REQUIRE_NOTHROW(regulating->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulating->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulating->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded FQG optimistic time constrains the subsequent TSO send floor",
    "[integration][development-profile][2025][time-management][tso]"
    "[fqr-optimistic-time-tso-send-floor][rti.service.flush-queue-request]"
    "[rti.service.send-interaction][rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador regulatingReports;
  ReportingFederateAmbassador peerReports;
  auto regulating = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "parameter-handle-provider-fom.xml").wstring();
  unsigned char const valueBytes[] = {0x46U, 0x51U, 0x52U, 0x44U};
  unsigned char const tagBytes[] = {0x47U, 0x52U, 0x41U, 0x4EU, 0x54U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(regulating->connect(regulatingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulating->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(regulating->joinFederationExecution(
      L"fqr-distinct-grant-regulating", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"fqr-distinct-grant-peer", L"publisher", federationName));

  auto const interactionClass = regulating->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = regulating->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE_NOTHROW(regulating->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(peer->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(regulating->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(peer->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(regulating->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(regulating->enableTimeConstrained());
  drainCallbacks(*regulating);
  REQUIRE_NOTHROW(regulating->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*regulating);
  REQUIRE_NOTHROW(peer->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*peer);

  // Move both regulators together so the FQR requester's GALT is 5. This
  // separates the actual grant (5) from the optimistic queued-message time (7).
  REQUIRE_NOTHROW(regulating->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  for (int pass = 0; pass != 8; ++pass) {
    drainCallbacks(*regulating);
    drainCallbacks(*peer);
  }
  rti1516_2025::HLAinteger64Time logicalTime;
  REQUIRE_NOTHROW(regulating->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 4);
  REQUIRE_NOTHROW(peer->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 4);

  ParameterHandleValueMap incomingValues;
  incomingValues.emplace(identifier, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const incoming = peer->sendInteraction(
      interactionClass, incomingValues, tag, rti1516_2025::HLAinteger64Time(7));
  REQUIRE(incoming.isValid());
  REQUIRE(regulatingReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(regulating->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  drainCallbacks(*regulating);
  REQUIRE(regulatingReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(regulatingReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(regulatingReports.timestampedInteractionReports.front().timeValue == L"7");
  REQUIRE(regulatingReports.flushQueueGrantReports.front().value == L"5");
  REQUIRE(regulatingReports.flushQueueGrantReports.front().optimisticValue == L"7");
  REQUIRE(regulatingReports.callbackOrder ==
          std::vector<std::string>{"interaction", "flush-grant"});

  REQUIRE_NOTHROW(regulating->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);
  ParameterHandleValueMap outgoingValues;
  outgoingValues.emplace(identifier, VariableLengthData(valueBytes, sizeof(valueBytes)));
  REQUIRE_THROWS_AS(
      regulating->sendInteraction(
          interactionClass,
          outgoingValues,
          tag,
          rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::InvalidLogicalTime);
  auto const outgoing = regulating->sendInteraction(
      interactionClass, outgoingValues, tag, rti1516_2025::HLAinteger64Time(8));
  REQUIRE(outgoing.isValid());
  REQUIRE_NOTHROW(regulating->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);
  REQUIRE_THROWS_AS(
      regulating->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::LogicalTimeAlreadyPassed);

  REQUIRE_NOTHROW(regulating->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulating->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulating->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded distinct FQR grant uses actual time for timestamped federation save bound",
    "[integration][development-profile][2025][time-management][save-restore]"
    "[fqr-distinct-actual-optimistic-federation-save-bound]"
    "[rti.service.request-federation-save][rti.service.flush-queue-request]"
    "[federate.callback.receive-interaction][federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador regulatingReports;
  ReportingFederateAmbassador peerReports;
  auto regulating = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "parameter-handle-provider-fom.xml").wstring();
  unsigned char const valueBytes[] = {0x46U, 0x51U, 0x52U, 0x53U};
  unsigned char const tagBytes[] = {0x53U, 0x41U, 0x56U, 0x45U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(regulating->connect(regulatingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulating->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(regulating->joinFederationExecution(
      L"fqr-save-bound-regulating", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"fqr-save-bound-peer", L"publisher", federationName));

  auto const interactionClass = regulating->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = regulating->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE_NOTHROW(regulating->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(peer->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(regulating->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(peer->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(regulating->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(regulating->enableTimeConstrained());
  drainCallbacks(*regulating);
  REQUIRE_NOTHROW(regulating->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*regulating);
  REQUIRE_NOTHROW(peer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*peer);

  REQUIRE_NOTHROW(regulating->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  for (int pass = 0; pass != 8; ++pass) {
    drainCallbacks(*regulating);
    drainCallbacks(*peer);
  }

  ParameterHandleValueMap incomingValues;
  incomingValues.emplace(identifier, VariableLengthData(valueBytes, sizeof(valueBytes)));
  REQUIRE(peer->sendInteraction(
              interactionClass,
              incomingValues,
              tag,
              rti1516_2025::HLAinteger64Time(7))
              .isValid());
  REQUIRE_NOTHROW(regulating->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  drainCallbacks(*regulating);
  REQUIRE(regulatingReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(regulatingReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(regulatingReports.flushQueueGrantReports.front().value == L"5");
  REQUIRE(regulatingReports.flushQueueGrantReports.front().optimisticValue == L"7");

  rti1516_2025::HLAinteger64Time logicalTime;
  REQUIRE_NOTHROW(regulating->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);
  REQUIRE_THROWS_AS(
      regulating->requestFederationSave(
          L"fqr-save-at-actual-time", rti1516_2025::HLAinteger64Time(5)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE_NOTHROW(regulating->requestFederationSave(
      L"fqr-save-above-actual-time", rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_THROWS_AS(
      regulating->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::LogicalTimeAlreadyPassed);

  REQUIRE_NOTHROW(regulating->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulating->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulating->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "A non-time-constrained federate receives an FQG without waiting for TSO delivery",
    "[integration][development-profile][2025][time-management]"
    "[flush-queue-request][non-time-constrained][no-tso-delivery]"
    "[rti.service.flush-queue-request][federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador producerReports;
  ReportingFederateAmbassador requesterReports;
  auto producer = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "parameter-handle-provider-fom.xml").wstring();
  unsigned char const valueBytes[] = {0x4EU, 0x4FU, 0x54U, 0x43U};
  unsigned char const tagBytes[] = {0x46U, 0x51U, 0x52U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(producer->connect(producerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(producer->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(producer->joinFederationExecution(
      L"fqr-non-time-constrained-producer", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"fqr-non-time-constrained-requester", L"subscriber", federationName));

  auto const interactionClass = producer->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = producer->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE_NOTHROW(producer->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(producer->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(requester->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(producer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*producer);

  ParameterHandleValueMap values;
  values.emplace(identifier, VariableLengthData(valueBytes, sizeof(valueBytes)));
  REQUIRE(producer->sendInteraction(
              interactionClass,
              values,
              tag,
              rti1516_2025::HLAinteger64Time(7))
              .isValid());
  REQUIRE(requesterReports.timestampedInteractionReports.empty());

  // The producer remains at time 0, so its in-transit timestamp-7 message
  // cannot be delivered by advancing it. The unconstrained requester must
  // therefore receive the message in receive order rather than await TSO.
  rti1516_2025::HLAinteger64Time producerTime;
  REQUIRE_NOTHROW(producer->queryLogicalTime(producerTime));
  REQUIRE(producerTime.getTime() == 0);
  REQUIRE_NOTHROW(requester->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(requesterReports.flushQueueGrantReports.empty());
  drainCallbacks(*requester);
  REQUIRE(requesterReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(requesterReports.timestampedInteractionReports.front().timeValue == L"7");
  REQUIRE(
      requesterReports.timestampedInteractionReports.front().sentOrderType == TIMESTAMP);
  REQUIRE(
      requesterReports.timestampedInteractionReports.front().receivedOrderType == RECEIVE);
  REQUIRE(requesterReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(requesterReports.callbackOrder ==
          std::vector<std::string>{"interaction", "flush-grant"});
  REQUIRE_NOTHROW(producer->queryLogicalTime(producerTime));
  REQUIRE(producerTime.getTime() == 0);

  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(producer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(producer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(producer->disconnect());
}
