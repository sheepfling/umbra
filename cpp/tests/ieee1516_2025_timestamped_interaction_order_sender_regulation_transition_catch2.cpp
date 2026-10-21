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
#error "The sender-regulation interaction order test requires the Umbra source directory."
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
using rti1516_2025::RegionHandleSet;
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
  return L"timestamped-interaction-sender-regulation-order-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

class InteractionOrderReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
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
      RegionHandleSet const*) override {
    ++untimedDeliveryCount;
    callbackOrder.push_back("untimed");
  }

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameters,
      VariableLengthData const& tag,
      TransportationTypeHandle const& transport,
      FederateHandle const& producer,
      RegionHandleSet const* sentRegions,
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
        retraction != nullptr && retraction->isValid(),
    });
    callbackOrder.push_back(sentOrder == TIMESTAMP ? "timestamp" : "receive");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    grants.push_back(time.toString());
    callbackOrder.push_back("grant");
  }

  void timeRegulationEnabled(LogicalTime const&) override {
    ++timeRegulationEnabledCount;
  }

  void timeConstrainedEnabled(LogicalTime const&) override {
    ++timeConstrainedEnabledCount;
  }

  std::vector<Delivery> deliveries;
  std::vector<std::wstring> grants;
  std::vector<std::string> callbackOrder;
  std::size_t untimedDeliveryCount = 0U;
  std::size_t timeRegulationEnabledCount = 0U;
  std::size_t timeConstrainedEnabledCount = 0U;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

template <typename Ready>
void evokeUntil(RTIambassador& rti, Ready ready) {
  for (unsigned attempt = 0U; attempt != 20U && !ready(); ++attempt) {
    static_cast<void>(rti.evokeCallback(0.25));
  }
}

}  // namespace

TEST_CASE(
    "Embedded timestamped interaction changes message order when sender becomes time-regulating",
    "[integration][development-profile][interaction-management][time-management]"
    "[timestamped-interaction-sender-regulation-order][tso][receive-order]"
    "[rti.service.send-interaction][rti.service.change-interaction-order-type]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant][2025]") {
  InteractionOrderReportingFederateAmbassador senderReports;
  InteractionOrderReportingFederateAmbassador receiverReports;
  InteractionOrderReportingFederateAmbassador regulatorReports;
  auto sender = makeRti();
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" /
      "tests" / "data" / "parameter-handle-provider-fom.xml";

  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName, fomModule.wstring(), standard_hla::mom::integer64_time));
  FederateHandle senderHandle;
  REQUIRE_NOTHROW(senderHandle = sender->joinFederationExecution(
      L"sender-regulation-interaction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"sender-regulation-interaction-receiver", L"subscriber", federationName));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(
      L"sender-regulation-interaction-regulator", L"regulator", federationName));

  auto const interactionClass = sender->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = sender->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  auto const receiverInteractionClass = receiver->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE(receiverInteractionClass.isValid());
  REQUIRE_NOTHROW(sender->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(receiverInteractionClass));

  REQUIRE_NOTHROW(regulator->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  evokeUntil(*regulator, [&] { return regulatorReports.timeRegulationEnabledCount == 1U; });
  REQUIRE(regulatorReports.timeRegulationEnabledCount == 1U);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  evokeUntil(*receiver, [&] { return receiverReports.timeConstrainedEnabledCount == 1U; });
  REQUIRE(receiverReports.timeConstrainedEnabledCount == 1U);
  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());

  auto const reliable = sender->getTransportationTypeHandle(standard_hla::mom::reliable);
  REQUIRE(reliable.isValid());
  ParameterHandleValueMap firstParameters;
  unsigned char const firstValueBytes[] = {0x41, 0x42};
  unsigned char const firstTagBytes[] = {0x31};
  firstParameters.emplace(
      identifier,
      VariableLengthData(firstValueBytes, sizeof(firstValueBytes)));
  VariableLengthData const firstTag(firstTagBytes, sizeof(firstTagBytes));

  // Table 2 of §8.1.4: preferred TSO plus a timestamp remains sent RO while
  // this sender is not regulating. A separate regulator holds GALT defined
  // so the supplied timestamp is a valid Send Interaction service input.
  auto const firstRetraction = sender->sendInteraction(
      interactionClass,
      firstParameters,
      firstTag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE_FALSE(firstRetraction.isValid());
  evokeUntil(*receiver, [&] { return receiverReports.deliveries.size() == 1U; });
  REQUIRE(receiverReports.untimedDeliveryCount == 0U);
  REQUIRE(receiverReports.deliveries.size() == 1U);
  REQUIRE(receiverReports.grants.empty());
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"receive"});
  auto const& first = receiverReports.deliveries.front();
  REQUIRE(first.interactionClass == receiverInteractionClass);
  REQUIRE(first.parameters.size() == 1U);
  REQUIRE(first.parameters.contains(identifier));
  REQUIRE(bytesOf(first.parameters.at(identifier)) ==
          std::vector<unsigned char>(firstValueBytes, firstValueBytes + sizeof(firstValueBytes)));
  REQUIRE(bytesOf(first.tag) ==
          std::vector<unsigned char>(firstTagBytes, firstTagBytes + sizeof(firstTagBytes)));
  REQUIRE(first.transport == reliable);
  REQUIRE(first.producer == senderHandle);
  REQUIRE_FALSE(first.sentRegionsSupplied);
  REQUIRE(first.timeImplementation == standard_hla::mom::integer64_time);
  REQUIRE(first.timeValue == L"6");
  REQUIRE(first.sentOrder == RECEIVE);
  REQUIRE(first.receivedOrder == RECEIVE);
  REQUIRE_FALSE(first.retractionSupplied);

  // Hold preferred order and timestamp fixed; enabling regulation is the only
  // change, so this send is TSO and returns a retraction designator.
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  evokeUntil(*sender, [&] { return senderReports.timeRegulationEnabledCount == 1U; });
  REQUIRE(senderReports.timeRegulationEnabledCount == 1U);
  unsigned char const secondValueBytes[] = {0x54, 0x53, 0x4F};
  unsigned char const secondTagBytes[] = {0x32};
  ParameterHandleValueMap secondParameters;
  secondParameters.emplace(
      identifier,
      VariableLengthData(secondValueBytes, sizeof(secondValueBytes)));
  VariableLengthData const secondTag(secondTagBytes, sizeof(secondTagBytes));
  auto const secondRetraction = sender->sendInteraction(
      interactionClass,
      secondParameters,
      secondTag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(secondRetraction.isValid());
  static_cast<void>(receiver->evokeCallback(0.1));
  REQUIRE(receiverReports.deliveries.size() == 1U);
  REQUIRE(receiverReports.grants.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(sender->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  static_cast<void>(sender->evokeCallback(0.0));
  static_cast<void>(regulator->evokeCallback(0.0));
  evokeUntil(*receiver, [&] {
    return receiverReports.deliveries.size() == 2U &&
        receiverReports.grants.size() == 1U;
  });
  REQUIRE(receiverReports.untimedDeliveryCount == 0U);
  REQUIRE(receiverReports.deliveries.size() == 2U);
  REQUIRE(receiverReports.grants == std::vector<std::wstring>{L"6"});
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"receive", "timestamp", "grant"});
  auto const& second = receiverReports.deliveries.back();
  REQUIRE(second.interactionClass == receiverInteractionClass);
  REQUIRE(second.parameters.size() == 1U);
  REQUIRE(second.parameters.contains(identifier));
  REQUIRE(bytesOf(second.parameters.at(identifier)) ==
          std::vector<unsigned char>(secondValueBytes, secondValueBytes + sizeof(secondValueBytes)));
  REQUIRE(bytesOf(second.tag) ==
          std::vector<unsigned char>(secondTagBytes, secondTagBytes + sizeof(secondTagBytes)));
  REQUIRE(second.transport == reliable);
  REQUIRE(second.producer == senderHandle);
  REQUIRE_FALSE(second.sentRegionsSupplied);
  REQUIRE(second.timeImplementation == standard_hla::mom::integer64_time);
  REQUIRE(second.timeValue == L"6");
  REQUIRE(second.sentOrder == TIMESTAMP);
  REQUIRE(second.receivedOrder == TIMESTAMP);
  REQUIRE(second.retractionSupplied);
  REQUIRE(second.retractionValid);

  static_cast<void>(receiver->evokeCallback(0.1));
  REQUIRE(receiverReports.deliveries.size() == 2U);
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}
