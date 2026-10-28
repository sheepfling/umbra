#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"
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
#error "The implicit regulation advance test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RECEIVE;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"time-regulation-implicit-advance-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

class TemporalReports final : public rti1516_2025::NullFederateAmbassador {
 public:
  struct Delivery final {
    InteractionClassHandle interactionClass;
    FederateHandle producer;
    std::wstring time;
    OrderType sentOrder = RECEIVE;
    OrderType receivedOrder = RECEIVE;
    bool retractionValid = false;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      FederateHandle const& producer,
      rti1516_2025::RegionHandleSet const*) override {
    deliveries.push_back({
        interactionClass,
        producer,
        L"",
        RECEIVE,
        RECEIVE,
        false});
    callbackOrder.push_back("interaction");
  }

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      FederateHandle const& producer,
      rti1516_2025::RegionHandleSet const*,
      LogicalTime const& time,
      OrderType sentOrder,
      OrderType receivedOrder,
      MessageRetractionHandle const* retraction) override {
    deliveries.push_back({
        interactionClass,
        producer,
        time.toString(),
        sentOrder,
        receivedOrder,
        retraction != nullptr && retraction->isValid()});
    callbackOrder.push_back("interaction");
  }

  void timeConstrainedEnabled(LogicalTime const& time) override {
    constrainedEnabled.push_back(time.toString());
  }

  void timeRegulationEnabled(LogicalTime const& time) override {
    regulationEnabled.push_back(time.toString());
    callbackOrder.push_back("regulation");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    grants.push_back(time.toString());
    callbackOrder.push_back("grant");
  }

  std::vector<Delivery> deliveries;
  std::vector<std::wstring> constrainedEnabled;
  std::vector<std::wstring> regulationEnabled;
  std::vector<std::wstring> grants;
  std::vector<std::string> callbackOrder;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded enabling regulation implicitly advances a constrained federate to the queued TSO boundary",
    "[integration][development-profile][time-management][tso]"
    "[time-regulation-implicit-advance]"
    "[time-regulation-smallest-feasible-enable-time]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[federate.callback.time-regulation-enabled][federate.callback.receive-interaction]") {
  TemporalReports producerReports;
  TemporalReports enablingReports;
  TemporalReports advancedPeerReports;
  auto producer = makeRti();
  auto enabling = makeRti();
  auto advancedPeer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "parameter-handle-provider-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(producer->connect(producerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(enabling->connect(enablingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(advancedPeer->connect(advancedPeerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(producer->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle producerHandle;
  REQUIRE_NOTHROW(producerHandle = producer->joinFederationExecution(
      L"implicit-advance-producer", L"publisher", federationName));
  REQUIRE_NOTHROW(enabling->joinFederationExecution(
      L"implicit-advance-enabling", L"subscriber", federationName));
  REQUIRE_NOTHROW(advancedPeer->joinFederationExecution(
      L"implicit-advance-peer", L"subscriber", federationName));

  auto const interactionClass = producer->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(producer->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(producer->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(enabling->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(enabling->enableTimeConstrained());
  REQUIRE_FALSE(enabling->evokeCallback(0.0));
  REQUIRE(enablingReports.constrainedEnabled == std::vector<std::wstring>{L"0"});
  REQUIRE_NOTHROW(advancedPeer->enableTimeConstrained());
  REQUIRE_FALSE(advancedPeer->evokeCallback(0.0));
  REQUIRE(advancedPeerReports.constrainedEnabled == std::vector<std::wstring>{L"0"});
  REQUIRE_NOTHROW(producer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(producer->evokeCallback(0.0));

  auto const retraction = producer->sendInteraction(
      interactionClass,
      ParameterHandleValueMap{},
      VariableLengthData{},
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(retraction.isValid());
  REQUIRE_FALSE(enabling->evokeCallback(0.0));
  REQUIRE(enablingReports.deliveries.empty());

  // With another constrained member at 10, lookahead 2 requires the new
  // regulator's assigned time to be at least 8. Its queued TSO at the equal
  // boundary remains eligible and precedes the callback, without an explicit
  // TAR/TAG pair.
  REQUIRE_NOTHROW(producer->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(producer->evokeCallback(0.0));
  REQUIRE(producerReports.grants == std::vector<std::wstring>{L"10"});
  REQUIRE_NOTHROW(advancedPeer->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(advancedPeer->evokeCallback(0.0));
  REQUIRE(advancedPeerReports.grants == std::vector<std::wstring>{L"10"});
  REQUIRE(enablingReports.deliveries.empty());

  REQUIRE_NOTHROW(enabling->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(2)));
  for (int attempt = 0;
       attempt != 8 && enablingReports.regulationEnabled.empty(); ++attempt) {
    enabling->evokeCallback(0.25);
  }
  REQUIRE(enablingReports.regulationEnabled.size() == 1);
  auto const enabledTime = std::stoll(enablingReports.regulationEnabled.front());
  REQUIRE(enabledTime == 8);
  REQUIRE(enablingReports.deliveries.size() == 1);
  REQUIRE(enablingReports.callbackOrder ==
          std::vector<std::string>{"interaction", "regulation"});
  REQUIRE(enablingReports.grants.empty());
  auto const& delivery = enablingReports.deliveries.front();
  REQUIRE(delivery.interactionClass == interactionClass);
  REQUIRE(delivery.producer == producerHandle);
  REQUIRE(delivery.time == L"8");
  REQUIRE(delivery.sentOrder == TIMESTAMP);
  REQUIRE(delivery.receivedOrder == TIMESTAMP);
  REQUIRE(delivery.retractionValid);
  enabling->evokeCallback(0.1);
  REQUIRE(enablingReports.deliveries.size() == 1);

  REQUIRE_NOTHROW(advancedPeer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(enabling->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(producer->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(producer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(advancedPeer->disconnect());
  REQUIRE_NOTHROW(enabling->disconnect());
  REQUIRE_NOTHROW(producer->disconnect());
}

TEST_CASE(
    "Embedded Send Message from Time Granted preserves the prior state",
    "[integration][development-profile][time-management][time-advance]"
    "[send-message-time-state-preservation]"
    "[rti.service.send-interaction][rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  TemporalReports senderReports;
  TemporalReports receiverReports;
  auto sender = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "parameter-handle-provider-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(sender->joinFederationExecution(
      L"send-state-sender", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"send-state-receiver", L"subscriber", federationName));

  auto const interactionClass = sender->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  auto const parameter = sender->getParameterHandle(
      interactionClass, umbra::test::hla::wide::fixture::identifier);
  REQUIRE(parameter.isValid());
  REQUIRE_NOTHROW(sender->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.constrainedEnabled == std::vector<std::wstring>{L"0"});
  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(sender->evokeCallback(0.0));
  REQUIRE(senderReports.regulationEnabled == std::vector<std::wstring>{L"0"});

  // Enter Time Granted, then exercise the Send Message state-chart transition.
  REQUIRE_NOTHROW(sender->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(10)));
  for (int attempt = 0;
       attempt != 8 && senderReports.grants.empty(); ++attempt) {
    sender->evokeCallback(0.25);
  }
  REQUIRE(senderReports.grants == std::vector<std::wstring>{L"10"});
  rti1516_2025::HLAinteger64Time senderTime(0);
  REQUIRE_NOTHROW(sender->queryLogicalTime(senderTime));
  REQUIRE(senderTime.toString() == L"10");

  // Send Message is permitted while Time Granted and returns the federate to
  // that state. The next TAR proves the message did not displace the state.
  unsigned char const payloadByte = 0x41U;
  ParameterHandleValueMap values;
  values.emplace(parameter, VariableLengthData(&payloadByte, sizeof(payloadByte)));
  REQUIRE_NOTHROW(sender->sendInteraction(
      interactionClass, values, VariableLengthData{}));
  for (int attempt = 0;
       attempt != 8 && receiverReports.deliveries.empty(); ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.deliveries.size() == 1);
  REQUIRE(receiverReports.deliveries.front().interactionClass == interactionClass);
  REQUIRE(receiverReports.deliveries.front().producer.isValid());
  REQUIRE(receiverReports.deliveries.front().sentOrder == RECEIVE);
  REQUIRE(receiverReports.deliveries.front().receivedOrder == RECEIVE);
  REQUIRE(receiverReports.grants.empty());
  REQUIRE(senderReports.grants == std::vector<std::wstring>{L"10"});
  REQUIRE_NOTHROW(sender->queryLogicalTime(senderTime));
  REQUIRE(senderTime.toString() == L"10");

  REQUIRE_NOTHROW(sender->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(20)));
  for (int attempt = 0; attempt != 8 && senderReports.grants.size() < 2U;
       ++attempt) {
    sender->evokeCallback(0.25);
  }
  REQUIRE(senderReports.grants ==
      std::vector<std::wstring>{L"10", L"20"});

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}
