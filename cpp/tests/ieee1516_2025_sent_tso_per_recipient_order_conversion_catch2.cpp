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
#error "The per-recipient order conversion test requires the Umbra source directory."
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
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::vector<unsigned char> bytesOf(VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  return bytes == nullptr ? std::vector<unsigned char>{}
                          : std::vector<unsigned char>{bytes, bytes + value.size()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"sent-tso-per-recipient-order-conversion-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

class InteractionReports final : public rti1516_2025::NullFederateAmbassador {
 public:
  struct Delivery final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameters;
    VariableLengthData tag;
    TransportationTypeHandle transport;
    FederateHandle producer;
    bool sentRegionsSupplied = false;
    std::wstring timeImplementation;
    std::wstring timeValue;
    OrderType sentOrder = RECEIVE;
    OrderType receivedOrder = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  void receiveInteraction(
      InteractionClassHandle const&,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      rti1516_2025::RegionHandleSet const*) override {
    ++untimedDeliveries;
    callbackOrder.push_back("untimed");
  }

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameters,
      VariableLengthData const& tag,
      TransportationTypeHandle const& transport,
      FederateHandle const& producer,
      rti1516_2025::RegionHandleSet const* sentRegions,
      LogicalTime const& time,
      OrderType sentOrder,
      OrderType receivedOrder,
      MessageRetractionHandle const* retraction) override {
    deliveries.push_back({
        interactionClass,
        parameters,
        tag,
        transport,
        producer,
        sentRegions != nullptr,
        time.implementationName(),
        time.toString(),
        sentOrder,
        receivedOrder,
        retraction != nullptr,
        retraction != nullptr && retraction->isValid()});
    callbackOrder.push_back("interaction");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    grants.push_back(time.toString());
    callbackOrder.push_back("grant");
  }

  std::vector<Delivery> deliveries;
  std::vector<std::wstring> grants;
  std::vector<std::string> callbackOrder;
  int untimedDeliveries = 0;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded one sent TSO interaction converts order per receiving federate",
    "[integration][development-profile][interaction-management][time-management]"
    "[sent-tso-per-recipient-order-conversion][tso][receive-order]"
    "[rti.service.send-interaction][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]") {
  InteractionReports senderReports;
  InteractionReports constrainedReports;
  InteractionReports unconstrainedReports;
  auto sender = makeRti();
  auto constrained = makeRti();
  auto unconstrained = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "parameter-handle-provider-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(unconstrained->connect(unconstrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle senderHandle;
  REQUIRE_NOTHROW(senderHandle = sender->joinFederationExecution(
      L"per-recipient-order-sender", L"publisher", federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"per-recipient-order-constrained", L"subscriber", federationName));
  REQUIRE_NOTHROW(unconstrained->joinFederationExecution(
      L"per-recipient-order-unconstrained", L"subscriber", federationName));

  auto const interactionClass = sender->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = sender->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  auto const reliable = sender->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE_NOTHROW(sender->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(constrained->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(unconstrained->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(sender->evokeCallback(0.0));

  unsigned char const payloadBytes[] = {0x54, 0x53, 0x4F};
  unsigned char const tagBytes[] = {0x50, 0x52};
  ParameterHandleValueMap parameters;
  parameters.emplace(
      identifier, VariableLengthData(payloadBytes, sizeof(payloadBytes)));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  // §8.1.4 Table 3: the same sent TSO message is received as RO by the
  // nonconstrained joined federate, but remains TSO for the constrained one.
  // Both recipients retain the designator because the sent message was TSO.
  auto const retraction = sender->sendInteraction(
      interactionClass, parameters, tag, rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  for (int attempt = 0;
       attempt != 8 && unconstrainedReports.deliveries.empty(); ++attempt) {
    unconstrained->evokeCallback(0.25);
  }
  constrained->evokeCallback(0.1);
  REQUIRE(unconstrainedReports.untimedDeliveries == 0);
  REQUIRE(unconstrainedReports.deliveries.size() == 1);
  REQUIRE(unconstrainedReports.grants.empty());
  REQUIRE(unconstrainedReports.callbackOrder ==
          std::vector<std::string>{"interaction"});
  REQUIRE(constrainedReports.untimedDeliveries == 0);
  REQUIRE(constrainedReports.deliveries.empty());
  REQUIRE(constrainedReports.grants.empty());

  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(sender->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(sender->evokeCallback(0.0));
  for (int attempt = 0;
       attempt != 8 && (constrainedReports.deliveries.empty() ||
                        constrainedReports.grants.empty()); ++attempt) {
    constrained->evokeCallback(0.25);
  }
  REQUIRE(constrainedReports.untimedDeliveries == 0);
  REQUIRE(constrainedReports.deliveries.size() == 1);
  REQUIRE(constrainedReports.grants == std::vector<std::wstring>{L"6"});
  REQUIRE(constrainedReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});

  auto requireCommon = [&](InteractionReports::Delivery const& delivery) {
    REQUIRE(delivery.interactionClass == interactionClass);
    REQUIRE(delivery.parameters.size() == 1);
    REQUIRE(delivery.parameters.contains(identifier));
    REQUIRE(bytesOf(delivery.parameters.at(identifier)) ==
            std::vector<unsigned char>(
                payloadBytes, payloadBytes + sizeof(payloadBytes)));
    REQUIRE(bytesOf(delivery.tag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(delivery.transport == reliable);
    REQUIRE(delivery.producer == senderHandle);
    REQUIRE_FALSE(delivery.sentRegionsSupplied);
    REQUIRE(delivery.timeImplementation == standard_hla::mom::integer64_time);
    REQUIRE(delivery.timeValue == L"6");
    REQUIRE(delivery.sentOrder == TIMESTAMP);
    REQUIRE(delivery.retractionSupplied);
    REQUIRE(delivery.retractionValid);
  };
  auto const& ro = unconstrainedReports.deliveries.front();
  auto const& tso = constrainedReports.deliveries.front();
  requireCommon(ro);
  requireCommon(tso);
  REQUIRE(ro.receivedOrder == RECEIVE);
  REQUIRE(tso.receivedOrder == TIMESTAMP);

  unconstrained->evokeCallback(0.1);
  constrained->evokeCallback(0.1);
  REQUIRE(unconstrainedReports.deliveries.size() == 1);
  REQUIRE(constrainedReports.deliveries.size() == 1);
  REQUIRE_NOTHROW(unconstrained->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(constrained->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unconstrained->disconnect());
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}

TEST_CASE(
    "Embedded timestamped Send Interaction converts to the interaction class preferred RECEIVE order",
    "[integration][development-profile][interaction-management][time-management]"
    "[send-message-preferred-order-conversion][receive-order]"
    "[rti.service.send-interaction][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]") {
  InteractionReports senderReports;
  InteractionReports receiverReports;
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
  FederateHandle senderHandle;
  REQUIRE_NOTHROW(senderHandle = sender->joinFederationExecution(
      L"preferred-receive-sender", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"preferred-receive-receiver", L"subscriber", federationName));

  auto const interactionClass = sender->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = sender->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  auto const reliable = sender->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE_NOTHROW(sender->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  // The FOM default is RECEIVE; set it explicitly so this test has one clear
  // preferred-order input while the timestamped overload supplies the other.
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, RECEIVE));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(sender->evokeCallback(0.0));

  unsigned char const payloadBytes[] = {0x52, 0x4F};
  unsigned char const tagBytes[] = {0x50, 0x52};
  ParameterHandleValueMap parameters{
      {identifier, VariableLengthData(payloadBytes, sizeof(payloadBytes))}};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const retraction = sender->sendInteraction(
      interactionClass, parameters, tag, rti1516_2025::HLAinteger64Time(6));
  // With preferred RECEIVE order, supplying time does not make this a TSO
  // message; its service result therefore has no retraction designator.
  REQUIRE_FALSE(retraction.isValid());
  for (int attempt = 0;
       attempt != 8 && receiverReports.deliveries.empty(); ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.untimedDeliveries == 0);
  REQUIRE(receiverReports.deliveries.size() == 1);
  REQUIRE(receiverReports.grants.empty());
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"interaction"});

  auto const& delivery = receiverReports.deliveries.front();
  REQUIRE(delivery.interactionClass == interactionClass);
  REQUIRE(delivery.parameters.size() == 1);
  REQUIRE(delivery.parameters.contains(identifier));
  REQUIRE(bytesOf(delivery.parameters.at(identifier)) ==
          std::vector<unsigned char>(
              payloadBytes, payloadBytes + sizeof(payloadBytes)));
  REQUIRE(bytesOf(delivery.tag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(delivery.transport == reliable);
  REQUIRE(delivery.producer == senderHandle);
  REQUIRE_FALSE(delivery.sentRegionsSupplied);
  REQUIRE(delivery.timeImplementation == standard_hla::mom::integer64_time);
  REQUIRE(delivery.timeValue == L"6");
  REQUIRE(delivery.sentOrder == RECEIVE);
  REQUIRE(delivery.receivedOrder == RECEIVE);
  REQUIRE_FALSE(delivery.retractionSupplied);
  REQUIRE_FALSE(delivery.retractionValid);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(sender->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(sender->evokeCallback(0.0));
  for (int attempt = 0;
       attempt != 8 && receiverReports.grants.empty(); ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.deliveries.size() == 1);
  REQUIRE(receiverReports.grants == std::vector<std::wstring>{L"6"});
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});
  receiver->evokeCallback(0.1);
  REQUIRE(receiverReports.deliveries.size() == 1);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}
