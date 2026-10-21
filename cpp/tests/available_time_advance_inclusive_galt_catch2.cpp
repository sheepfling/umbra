#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The inclusive-GALT available-time test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RECEIVE;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"available-time-advance-inclusive-galt-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path sourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  explicit ReportingFederateAmbassador(
      bool recordTimeConstrainedCallbackOrder = false) noexcept
      : recordTimeConstrainedCallbackOrder_(recordTimeConstrainedCallbackOrder) {}

  struct TimestampedInteractionReport final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
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
    std::vector<unsigned char> userSuppliedTag;
  };

  struct TimeAdvanceGrantReport final {
    std::wstring timeImplementationName;
    std::wstring value;
  };

  struct ReceiveOrderAttributeReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    FederateHandle producingFederate;
    std::vector<unsigned char> userSuppliedTag;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle const* optionalRetraction) override {
    timestampedInteractionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("interaction");
  }

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const&,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const&,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*) override {
    std::vector<unsigned char> tagBytes(userSuppliedTag.size());
    if (!tagBytes.empty()) {
      auto const* data = static_cast<unsigned char const*>(userSuppliedTag.data());
      if (data != nullptr) {
        std::copy(data, data + tagBytes.size(), tagBytes.begin());
      }
    }
    receiveOrderInteractionReports.push_back(
        {interactionClass, producingFederate, std::move(tagBytes)});
    if (recordTimeConstrainedCallbackOrder_) {
      callbackOrder.push_back("receive-order-interaction");
    }
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const&,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*) override {
    std::vector<unsigned char> tagBytes(userSuppliedTag.size());
    if (!tagBytes.empty()) {
      auto const* data =
          static_cast<unsigned char const*>(userSuppliedTag.data());
      if (data != nullptr) {
        std::copy(data, data + tagBytes.size(), tagBytes.begin());
      }
    }
    receiveOrderAttributeReports.push_back({
        objectInstance,
        attributeValues,
        producingFederate,
        std::move(tagBytes),
    });
    callbackOrder.push_back("receive-order-attribute");
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const&,
      AttributeHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      rti1516_2025::RegionHandleSet const*,
      LogicalTime const&,
      OrderType,
      OrderType,
      MessageRetractionHandle const*) override {
    ++timestampedAttributeReflectionCount;
    callbackOrder.push_back("timestamped-attribute");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  void timeRegulationEnabled(LogicalTime const& time) override {
    timeRegulationEnabledReports.push_back(
        {time.implementationName(), time.toString()});
  }

  void timeConstrainedEnabled(LogicalTime const& time) override {
    timeConstrainedEnabledReports.push_back(
        {time.implementationName(), time.toString()});
    if (recordTimeConstrainedCallbackOrder_) {
      callbackOrder.push_back("time-constrained-enabled");
    }
  }

  std::vector<TimestampedInteractionReport> timestampedInteractionReports;
  std::vector<ReceiveOrderInteractionReport> receiveOrderInteractionReports;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<TimeAdvanceGrantReport> timeRegulationEnabledReports;
  std::vector<TimeAdvanceGrantReport> timeConstrainedEnabledReports;
  std::vector<ReceiveOrderAttributeReport> receiveOrderAttributeReports;
  std::size_t timestampedAttributeReflectionCount = 0U;
  std::vector<std::string> callbackOrder;

 private:
  bool recordTimeConstrainedCallbackOrder_ = false;
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
    "Embedded Available time advances use inclusive GALT and queued TSO delivery",
    "[integration][development-profile][interaction-management][time-management]"
    "[time-advance-request-available][next-message-request-available]"
    "[rti.service.time-advance-request-available][rti.service.next-message-request-available]"
    "[rti.service.send-interaction][rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x41, 0x56, 0x41};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"available-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"available-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xA1, 0xB2};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);

  auto const first = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(first.isValid());

  // TARA reaches the exact defined GALT boundary after the regulator moves
  // to logical time 2. The message at 7 is delivered before the grant.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});

  // NMRA also selects the next queued timestamp, but its inclusive GALT rule
  // is exercised without relying on the message itself to mark the boundary:
  // the regulator advances from 2 to 4, making GALT exactly 9.
  auto const second = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(9));
  REQUIRE(second.isValid());
  REQUIRE_NOTHROW(receiver->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);
  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant", "interaction", "grant"});
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"9");
  REQUIRE(receiverReports.timestampedInteractionReports.back().timeValue == L"9");

  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(receiver->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 9);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Time Constrained transition preserves queued TSO at or beyond logical time",
    "[integration][development-profile][interaction-management][time-management]"
    "[lane.tce-transition-stored-tso][rti.service.enable-time-constrained]"
    "[rti.service.send-interaction][rti.service.time-advance-request]"
    "[federate.callback.time-constrained-enabled]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador regulatorReports;
  ReportingFederateAmbassador receiverReports{true};
  auto regulator = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName() + L"-tce-stored-tso";
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x54, 0x43, 0x45, 0x2D, 0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(
      L"tce-stored-tso-regulator", L"regulator", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"tce-stored-tso-receiver", L"subscriber", federationName));

  auto const interactionClass = regulator->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = regulator->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0x54, 0x53, 0x4F};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(regulator->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(regulator->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*regulator);
  REQUIRE(regulatorReports.timeRegulationEnabledReports.size() == 1U);

  // Move the unconstrained receiver beyond the regulator's current GALT, then
  // leave Enable Time Constrained pending until GALT advances past time 5.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"5");
  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeConstrainedEnabledReports.empty());

  // This timestamped message arrives while the role transition is pending.
  // Once the receiver becomes constrained, the queued TSO at time 6 must stay
  // timestamp-order rather than being emitted as a pre-transition RO callback.
  auto const queuedTso = regulator->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(queuedTso.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*regulator);
  // Do not evoke the receiver while its Enable Time Constrained request is
  // still pending: a public callback before the role transition belongs to
  // the pre-transition receive-order cohort, not the stored-TSO cohort.
  REQUIRE(receiverReports.timeConstrainedEnabledReports.empty());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  drainCallbacks(*regulator);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(receiverReports.timeConstrainedEnabledReports.back().value == L"5");
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.timestampedInteractionReports.front().timeValue == L"6");
  REQUIRE(receiverReports.timestampedInteractionReports.front().sentOrderType == TIMESTAMP);
  REQUIRE(receiverReports.timestampedInteractionReports.front().receivedOrderType == TIMESTAMP);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"6");
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{
      "time-constrained-enabled", "interaction", "grant"});

  REQUIRE_NOTHROW(receiver->disableTimeConstrained());
  REQUIRE_NOTHROW(regulator->disableTimeRegulation());
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(regulator->disconnect());
}

TEST_CASE(
    "Time Regulation enable suppresses TSO below its assigned federation time",
    "[integration][development-profile][interaction-management][time-management]"
    "[lane.time-regulation-enable-tso-cutoff][time-regulation-enable]"
    "[rti.service.enable-time-regulation][rti.service.send-interaction]"
    "[federate.callback.receive-interaction][federate.callback.time-regulation-enabled]") {
  ReportingFederateAmbassador producerReports;
  ReportingFederateAmbassador enablingReports;
  ReportingFederateAmbassador advancedPeerReports;
  auto producer = makeRti();
  auto enabling = makeRti();
  auto advancedPeer = makeRti();
  auto const federationName = nextFederationName() + L"-regulation-tso-cutoff";
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x52, 0x54, 0x43};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(producer->connect(producerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(enabling->connect(enablingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(advancedPeer->connect(advancedPeerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(producer->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(producer->joinFederationExecution(
      L"regulation-cutoff-producer", L"publisher", federationName));
  REQUIRE_NOTHROW(enabling->joinFederationExecution(
      L"regulation-cutoff-enabling", L"subscriber", federationName));
  REQUIRE_NOTHROW(advancedPeer->joinFederationExecution(
      L"regulation-cutoff-advanced-peer", L"subscriber", federationName));

  auto const interactionClass = producer->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(producer->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(producer->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(enabling->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(enabling->enableTimeConstrained());
  drainCallbacks(*enabling);
  REQUIRE_NOTHROW(advancedPeer->enableTimeConstrained());
  drainCallbacks(*advancedPeer);
  REQUIRE_NOTHROW(producer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*producer);

  // Both messages queue for the constrained receiver. The future transition
  // will establish time 8: TSO(5) must be suppressed, while TSO(8) remains on
  // the inclusive TARA boundary and may be delivered before the callback.
  REQUIRE(producer->sendInteraction(
      interactionClass,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(5)).isValid());
  REQUIRE(producer->sendInteraction(
      interactionClass,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(8)).isValid());
  REQUIRE(enablingReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(producer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(10)));
  drainCallbacks(*producer);
  REQUIRE(producerReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(producerReports.timeAdvanceGrantReports.front().value == L"10");
  REQUIRE_NOTHROW(advancedPeer->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(10)));
  drainCallbacks(*advancedPeer);
  REQUIRE(advancedPeerReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(advancedPeerReports.timeAdvanceGrantReports.front().value == L"10");
  REQUIRE(enablingReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(enabling->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(2)));
  drainCallbacks(*enabling);
  REQUIRE(enablingReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE(enablingReports.timeRegulationEnabledReports.front().value == L"8");
  REQUIRE(enablingReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(enablingReports.timestampedInteractionReports.front().timeValue == L"8");
  REQUIRE(enablingReports.timestampedInteractionReports.front().sentOrderType == TIMESTAMP);
  REQUIRE(enablingReports.timestampedInteractionReports.front().receivedOrderType == TIMESTAMP);
  REQUIRE(enablingReports.timeAdvanceGrantReports.empty());

  REQUIRE_NOTHROW(advancedPeer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(enabling->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(producer->resignFederationExecution(rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(producer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(advancedPeer->disconnect());
  REQUIRE_NOTHROW(enabling->disconnect());
  REQUIRE_NOTHROW(producer->disconnect());
}

TEST_CASE(
    "Embedded no-TSO NMR and NMRA grants wait on cross-federate GALT",
    "[integration][development-profile][time-management][time-advance]"
    "[next-message-request][next-message-request-available][galt][scheduler]"
    "[no-tso-cross-federate-grant-scheduler]"
    "[rti.service.next-message-request][rti.service.next-message-request-available]"
    "[rti.service.time-advance-request][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador regulatorReports;
  ReportingFederateAmbassador requesterReports;
  auto regulator = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName() + L"-no-tso-scheduler";
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();

  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(
      L"no-tso-regulator", L"regulator", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"no-tso-requester", L"requester", federationName));

  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drainCallbacks(*requester);
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*regulator);

  // With no queued TSO input, NMR retains the supplied target and waits at
  // the strict regulator-only GALT of 5. Advancing the other member moves
  // GALT to 6 and releases the accepted request at 5.
  REQUIRE_NOTHROW(requester->nextMessageRequest(
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(requesterReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  drainCallbacks(*regulator);
  drainCallbacks(*requester);
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.back().value == L"5");

  // NMRA also has no TSO candidate. A request for 7 waits while GALT is 6,
  // then releases exactly at the inclusive boundary when the regulator moves
  // to 2. This exercises the same scheduler through the Available service.
  REQUIRE_NOTHROW(requester->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*regulator);
  drainCallbacks(*requester);
  REQUIRE(regulatorReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.back().value == L"7");
  REQUIRE(requesterReports.callbackOrder ==
          std::vector<std::string>{"grant", "grant"});

  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(requester->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 7);

  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(regulator->disconnect());
}

TEST_CASE(
    "Embedded no-TSO TARA waits for cross-federate GALT equality",
    "[integration][development-profile][time-management][time-advance]"
    "[time-advance-request-available][galt][scheduler]"
    "[no-tso-cross-federate-tara-galt-boundary]"
    "[rti.service.time-advance-request-available]"
    "[rti.service.time-advance-request][rti.service.query-galt]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador regulatorReports;
  ReportingFederateAmbassador requesterReports;
  auto regulator = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName() + L"-no-tso-tara-galt";
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();

  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(
      L"no-tso-tara-regulator", L"regulator", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"no-tso-tara-requester", L"requester", federationName));

  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drainCallbacks(*requester);
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*regulator);

  rti1516_2025::HLAinteger64Time galt;
  REQUIRE_NOTHROW(requester->queryGALT(galt));
  REQUIRE(galt.getTime() == 5);

  // TARA(7) remains pending while GALT is 5. The regulator's TAR(2) moves
  // GALT to exactly 7, which must release the available-time request.
  REQUIRE_NOTHROW(requester->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE(requesterReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*regulator);
  drainCallbacks(*requester);

  REQUIRE_NOTHROW(requester->queryGALT(galt));
  REQUIRE(galt.getTime() == 7);
  REQUIRE(regulatorReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(regulatorReports.timeAdvanceGrantReports.back().value == L"2");
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.back().value == L"7");
  REQUIRE(requesterReports.callbackOrder == std::vector<std::string>{"grant"});

  rti1516_2025::HLAinteger64Time requesterTime;
  REQUIRE_NOTHROW(requester->queryLogicalTime(requesterTime));
  REQUIRE(requesterTime.getTime() == 7);

  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(regulator->disconnect());
}

TEST_CASE(
    "Embedded NMR grants below defined GALT without another federate advance",
    "[integration][development-profile][time-management][time-advance]"
    "[next-message-request][galt][nmr-galt-immediate-grant]"
    "[rti.service.next-message-request][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador regulatorReports;
  ReportingFederateAmbassador requesterReports;
  auto regulator = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName() + L"-nmr-below-galt";
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();

  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(
      L"nmr-below-galt-regulator", L"regulator", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"nmr-below-galt-requester", L"requester", federationName));

  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drainCallbacks(*requester);
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*regulator);

  // The regulator remains at logical time 0, but its lookahead establishes
  // GALT 5. NMR(3) is strictly below GALT and must not wait for a TAR.
  REQUIRE(regulatorReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(requester->nextMessageRequest(
      rti1516_2025::HLAinteger64Time(3)));
  drainCallbacks(*requester);
  REQUIRE(regulatorReports.timeAdvanceGrantReports.empty());
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.front().value == L"3");

  rti1516_2025::HLAinteger64Time regulatorTime;
  rti1516_2025::HLAinteger64Time requesterTime;
  REQUIRE_NOTHROW(regulator->queryLogicalTime(regulatorTime));
  REQUIRE_NOTHROW(requester->queryLogicalTime(requesterTime));
  REQUIRE(regulatorTime.getTime() == 0);
  REQUIRE(requesterTime.getTime() == 3);

  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(regulator->disconnect());
}

TEST_CASE(
    "Embedded NMR grants at a queued message below GALT without another federate advance",
    "[integration][development-profile][interaction-management][time-management]"
    "[next-message-request][galt][lits-below-galt-immediate-grant]"
    "[rti.service.next-message-request][rti.service.send-interaction]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador regulatorReports;
  ReportingFederateAmbassador requesterReports;
  auto regulator = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName() + L"-nmr-lits-below-galt";
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x4EU, 0x4DU, 0x52U};
  unsigned char const identifierBytes[] = {0x4CU, 0x49, 0x54, 0x53};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(
      L"nmr-lits-regulator", L"regulator", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"nmr-lits-requester", L"requester", federationName));

  auto const interactionClass = regulator->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const parameter = regulator->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE_NOTHROW(regulator->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(regulator->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(requester->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drainCallbacks(*requester);
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*regulator);

  ParameterHandleValueMap values;
  values.emplace(parameter,
                 VariableLengthData(identifierBytes, sizeof(identifierBytes)));
  REQUIRE(regulator->sendInteraction(
      interactionClass,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(3)).isValid());
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*regulator);
  drainCallbacks(*requester);

  // The TSO(3) is queued for the constrained requester. Once the regulator
  // reaches 4 with lookahead 1, LITS is 3 < GALT 5; NMR(10) can grant at 3
  // without another regulator advance.
  REQUIRE(regulatorReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(regulatorReports.timeAdvanceGrantReports.front().value == L"4");
  REQUIRE(requesterReports.timestampedInteractionReports.empty());
  REQUIRE(requesterReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(requester->nextMessageRequest(
      rti1516_2025::HLAinteger64Time(10)));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(requesterReports.timestampedInteractionReports.front().timeValue == L"3");
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.front().value == L"3");
  REQUIRE(requesterReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});

  rti1516_2025::HLAinteger64Time regulatorTime;
  rti1516_2025::HLAinteger64Time requesterTime;
  REQUIRE_NOTHROW(regulator->queryLogicalTime(regulatorTime));
  REQUIRE_NOTHROW(requester->queryLogicalTime(requesterTime));
  REQUIRE(regulatorTime.getTime() == 4);
  REQUIRE(requesterTime.getTime() == 3);

  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(regulator->disconnect());
}

TEST_CASE(
    "Time advance grant reports the requested logical time",
    "[integration][development-profile][time-management][time-advance]"
    "[time-advance-request][galt][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador regulatorReports;
  ReportingFederateAmbassador requesterReports;
  auto regulator = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName() + L"-grant-requested-time";
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();

  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(
      L"grant-time-regulator", L"regulator", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"grant-time-requester", L"requester", federationName));

  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drainCallbacks(*requester);
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*regulator);

  REQUIRE_NOTHROW(requester->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE(requesterReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  drainCallbacks(*regulator);
  drainCallbacks(*requester);
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.front().value == L"7");

  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(requester->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 7);

  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(regulator->disconnect());
}

TEST_CASE(
    "Time Constrained Enabled reports current logical time for a regulating federate",
    "[integration][development-profile][time-management][time-role]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-constrained-enabled]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName() + L"-constrained-regulating-time";
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"constrained-regulating-federate", L"time-manager", federationName));

  REQUIRE_NOTHROW(rti->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*rti);
  REQUIRE(reports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE(reports.timeRegulationEnabledReports.front().value == L"0");

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  drainCallbacks(*rti);
  REQUIRE(reports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(reports.timeAdvanceGrantReports.front().value == L"5");

  REQUIRE_NOTHROW(rti->enableTimeConstrained());
  drainCallbacks(*rti);
  REQUIRE(reports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(reports.timeConstrainedEnabledReports.front().value == L"5");

  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(rti->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 5);

  REQUIRE_NOTHROW(rti->disableTimeConstrained());
  REQUIRE_NOTHROW(rti->disableTimeRegulation());
  REQUIRE_NOTHROW(rti->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Time Constrained enable waits until GALT exceeds current logical time",
    "[integration][development-profile][time-management][time-role]"
    "[galt][scheduler][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][federate.callback.time-constrained-enabled]") {
  ReportingFederateAmbassador regulatorReports;
  ReportingFederateAmbassador requesterReports;
  auto regulator = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName() + L"-constrained-enable-galt";
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();

  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(
      L"constrained-enable-regulator", L"regulator", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"constrained-enable-requester", L"requester", federationName));

  REQUIRE_NOTHROW(regulator->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*regulator);
  REQUIRE(regulatorReports.timeRegulationEnabledReports.size() == 1U);

  // This requester is not time constrained yet, so it may advance to 5 while
  // the regulating member's GALT remains 1.
  REQUIRE_NOTHROW(requester->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.back().value == L"5");

  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drainCallbacks(*requester);
  REQUIRE(requesterReports.timeConstrainedEnabledReports.empty());

  // Advancing the regulator to 4 moves GALT to 5, exactly the requester's
  // current logical time. Equality is insufficient: even after evoking, the
  // pending role transition must remain disabled.
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*regulator);
  drainCallbacks(*requester);
  REQUIRE(requesterReports.timeConstrainedEnabledReports.empty());

  // GALT=6 is now strictly beyond the unchanged logical time 5. The callback
  // becomes eligible and remains queued until the requester evokes it.
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  drainCallbacks(*regulator);
  REQUIRE(requesterReports.timeConstrainedEnabledReports.empty());
  drainCallbacks(*requester);
  REQUIRE(requesterReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(requesterReports.timeConstrainedEnabledReports.back().value == L"5");
  rti1516_2025::HLAinteger64Time constrainedLogicalTime;
  REQUIRE_NOTHROW(requester->queryLogicalTime(constrainedLogicalTime));
  REQUIRE(constrainedLogicalTime.getTime() == 5);

  REQUIRE_NOTHROW(requester->disableTimeConstrained());
  REQUIRE_NOTHROW(regulator->disableTimeRegulation());
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(regulator->disconnect());
}

TEST_CASE(
    "Time Constrained transition preserves the receive-order boundary for queued interactions",
    "[integration][development-profile][interaction-management][time-management]"
    "[lane.tce-transition-delivery][rti.service.enable-time-constrained]"
    "[federate.callback.time-constrained-enabled][rti.service.send-interaction]"
    "[federate.callback.receive-interaction][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports{true};
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName() + L"-tce-transition-delivery";
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x54, 0x43, 0x45};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"tce-transition-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"tce-transition-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0x54, 0x43};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  // This timestamp is at or beyond the receiver's logical time, but it is
  // queued for callback delivery before the receiver enters Time Constrained.
  // The queued callback must stay receive-order and precede the transition
  // callback rather than being relabelled merely because TCE was requested.
  auto const beforeTransition = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(beforeTransition.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE(receiverReports.timeConstrainedEnabledReports.empty());
  drainCallbacks(*receiver);

  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.timestampedInteractionReports.front().timeValue == L"5");
  REQUIRE(receiverReports.timestampedInteractionReports.front().sentOrderType == TIMESTAMP);
  REQUIRE(receiverReports.timestampedInteractionReports.front().receivedOrderType == RECEIVE);
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "time-constrained-enabled"});

  // After the callback boundary, the next timestamped message is retained in
  // TSO until the constrained receiver advances through its timestamp.
  auto const afterTransition = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(afterTransition.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);

  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2U);
  REQUIRE(receiverReports.timestampedInteractionReports.back().timeValue == L"2");
  REQUIRE(receiverReports.timestampedInteractionReports.back().sentOrderType == TIMESTAMP);
  REQUIRE(receiverReports.timestampedInteractionReports.back().receivedOrderType == TIMESTAMP);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{
      "interaction", "time-constrained-enabled", "interaction", "grant"});

  REQUIRE_NOTHROW(receiver->disableTimeConstrained());
  REQUIRE_NOTHROW(publisher->disableTimeRegulation());
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Disable Time Constrained delivers queued and subsequent TSO as receive order",
    "[integration][development-profile][interaction-management][time-management]"
    "[lane.tcd-transition-tso-to-ro][time-constrained-disable-tso-to-ro]"
    "[rti.service.disable-time-constrained][rti.service.send-interaction]"
    "[federate.callback.receive-interaction][2025]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports{true};
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName() + L"-tcd-tso-to-ro";
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const queuedTagBytes[] = {0x51};
  VariableLengthData const queuedTag(queuedTagBytes, sizeof(queuedTagBytes));
  unsigned char const subsequentTagBytes[] = {0x53};
  VariableLengthData const subsequentTag(
      subsequentTagBytes, sizeof(subsequentTagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"tcd-tso-to-ro-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"tcd-tso-to-ro-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  auto const identifier = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  REQUIRE(identifier.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);

  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0x54, 0x44};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));
  auto const queued = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      queuedTag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(queued.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // Disabling Time Constrained changes queued TSO delivery to receive order.
  REQUIRE_NOTHROW(receiver->disableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE(receiverReports.receiveOrderInteractionReports.size() == 1U);
  REQUIRE(receiverReports.receiveOrderInteractionReports.front().interactionClass ==
          interactionClass);
  REQUIRE(receiverReports.receiveOrderInteractionReports.front().producingFederate.isValid());
  REQUIRE(receiverReports.receiveOrderInteractionReports.front().userSuppliedTag ==
          std::vector<unsigned char>{0x51});

  // TSO sent after the role transition is also received in receive order.
  auto const subsequent = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      subsequentTag,
      rti1516_2025::HLAinteger64Time(3));
  REQUIRE(subsequent.isValid());
  REQUIRE(receiverReports.receiveOrderInteractionReports.size() == 1U);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.timestampedInteractionReports.back().interactionClass ==
          interactionClass);
  REQUIRE(receiverReports.timestampedInteractionReports.back().timeValue == L"3");
  REQUIRE(receiverReports.timestampedInteractionReports.back().sentOrderType == TIMESTAMP);
  REQUIRE(receiverReports.timestampedInteractionReports.back().receivedOrderType == RECEIVE);
  auto const& deliveredTag =
      receiverReports.timestampedInteractionReports.back().userSuppliedTag;
  REQUIRE(deliveredTag.size() == sizeof(subsequentTagBytes));
  auto const* deliveredTagBytes =
      static_cast<unsigned char const*>(deliveredTag.data());
  REQUIRE(deliveredTagBytes != nullptr);
  REQUIRE(std::vector<unsigned char>(
              deliveredTagBytes,
              deliveredTagBytes + deliveredTag.size()) ==
          std::vector<unsigned char>{0x53});
  REQUIRE(receiverReports.receiveOrderInteractionReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{
      "time-constrained-enabled",
      "receive-order-interaction",
      "interaction"});

  REQUIRE_NOTHROW(publisher->disableTimeRegulation());
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Disable Time Constrained delivers queued timestamped attribute updates as receive order",
    "[integration][development-profile][object-management][time-management]"
    "[lane.tcd-transition-tso-to-ro][time-constrained-disable-tso-to-ro]"
    "[rti.service.disable-time-constrained][rti.service.update-attribute-values]"
    "[rti.service.evoke-callback][federate.callback.reflect-attribute-values]"
    "[requirement:hla-1516.1-2025:clause-8.7][2025]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports{true};
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName() + L"-tcd-attribute-tso-to-ro";
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" /
                                    "data" / "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const attributeBytes[] = {0x63U, 0x64U};
  unsigned char const tagBytes[] = {0x65U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  auto const publisherFederate = publisher->joinFederationExecution(
      L"tcd-attribute-publisher", L"publisher", federationName);
  REQUIRE(publisherFederate.isValid());
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"tcd-attribute-receiver", L"subscriber", federationName));

  auto const publisherClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const receiverClass = receiver->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const publisherAttribute = publisher->getAttributeHandle(
      publisherClass, fixture_hla::fixture::reliable_base_a);
  auto const receiverAttribute = receiver->getAttributeHandle(
      receiverClass, fixture_hla::fixture::reliable_base_a);
  REQUIRE(publisherClass.isValid());
  REQUIRE(receiverClass == publisherClass);
  REQUIRE(publisherAttribute.isValid());
  REQUIRE(receiverAttribute == publisherAttribute);

  AttributeHandleSet const publishedAttributes{publisherAttribute};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherClass, publishedAttributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      publisherClass, publishedAttributes, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      receiverClass, AttributeHandleSet{receiverAttribute}));
  auto const objectInstance = publisher->registerObjectInstance(publisherClass);
  REQUIRE(objectInstance.isValid());

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);

  AttributeHandleValueMap values;
  values.emplace(
      publisherAttribute,
      VariableLengthData(attributeBytes, sizeof(attributeBytes)));
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.receiveOrderAttributeReports.empty());
  REQUIRE(receiverReports.timestampedAttributeReflectionCount == 0U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  // The successful timestamped update has entered the federation queue before
  // this role transition; the queued reflection must switch to receive order.
  REQUIRE_NOTHROW(receiver->disableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.receiveOrderAttributeReports.size() == 1U);
  REQUIRE(receiverReports.timestampedAttributeReflectionCount == 0U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{
      "time-constrained-enabled", "receive-order-attribute"});

  auto const& delivered = receiverReports.receiveOrderAttributeReports.front();
  REQUIRE(delivered.objectInstance == objectInstance);
  REQUIRE(delivered.producingFederate == publisherFederate);
  REQUIRE(delivered.userSuppliedTag == std::vector<unsigned char>{0x65U});
  REQUIRE(delivered.attributeValues.size() == 1U);
  auto const deliveredValue = delivered.attributeValues.find(receiverAttribute);
  REQUIRE(deliveredValue != delivered.attributeValues.end());
  REQUIRE(deliveredValue->second.size() == sizeof(attributeBytes));
  auto const* deliveredBytes =
      static_cast<unsigned char const*>(deliveredValue->second.data());
  REQUIRE(deliveredBytes != nullptr);
  REQUIRE(std::vector<unsigned char>(
              deliveredBytes, deliveredBytes + deliveredValue->second.size()) ==
          std::vector<unsigned char>{0x63U, 0x64U});

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded non-regulating federate advancement does not constrain peer time advance",
    "[integration][development-profile][time-management][time-advance]"
    "[non-regulating-federate][peer-advance-independence]"
    "[rti.service.time-advance-request][rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained][federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-8][2025]") {
  ReportingFederateAmbassador nonRegulatingReports;
  ReportingFederateAmbassador regulatorReports;
  ReportingFederateAmbassador constrainedReports;
  auto nonRegulating = makeRti();
  auto regulator = makeRti();
  auto constrained = makeRti();
  auto const federationName = nextFederationName() + L"-nonregulating-peer-advance";
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();

  REQUIRE_NOTHROW(nonRegulating->connect(nonRegulatingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(nonRegulating->joinFederationExecution(
      L"non-regulating-peer", L"non-regulating", federationName));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(
      L"advancing-regulator", L"regulator", federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"time-constrained-peer", L"constrained", federationName));

  // Complete a time advance while this member is not time-regulating. Its
  // current time of 1 must not become a lower bound for another member's grant.
  REQUIRE_NOTHROW(nonRegulating->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  drainCallbacks(*nonRegulating);
  REQUIRE(nonRegulatingReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(nonRegulatingReports.timeAdvanceGrantReports.front().value == L"1");

  // Establish an independent regulating frontier at time 10 with lookahead 1.
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(10)));
  drainCallbacks(*regulator);
  REQUIRE(regulatorReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(regulatorReports.timeAdvanceGrantReports.front().value == L"10");
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*regulator);
  REQUIRE(regulatorReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE(regulatorReports.timeRegulationEnabledReports.front().value == L"10");

  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  drainCallbacks(*constrained);
  REQUIRE(constrainedReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(constrainedReports.timeConstrainedEnabledReports.front().value == L"0");

  // The constrained member may advance to 10 under the regulator's GALT=11;
  // the non-regulating member at time 1 has no effect on this advancement.
  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(10)));
  drainCallbacks(*constrained);
  REQUIRE(constrainedReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(constrainedReports.timeAdvanceGrantReports.front().value == L"10");

  REQUIRE_NOTHROW(constrained->disableTimeConstrained());
  REQUIRE_NOTHROW(regulator->disableTimeRegulation());
  REQUIRE_NOTHROW(nonRegulating->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(constrained->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(nonRegulating->disconnect());
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(regulator->disconnect());
}
