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
  struct UntimedDelivery final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameters;
    VariableLengthData tag;
    TransportationTypeHandle transport;
    FederateHandle producer;
    bool sentRegionsSupplied = false;
  };

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
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameters,
      VariableLengthData const& tag,
      TransportationTypeHandle const& transport,
      FederateHandle const& producer,
      RegionHandleSet const* sentRegions) override {
    untimedDeliveries.push_back({
        interactionClass,
        parameters,
        tag,
        transport,
        producer,
        sentRegions != nullptr,
    });
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
  std::vector<UntimedDelivery> untimedDeliveries;
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
    "[interaction-send-order-classification-matrix]"
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
  // Table 2 baseline: RECEIVE preference, no sender regulation, no timestamp.
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, RECEIVE));
  unsigned char const receiveUntimedValueBytes[] = {0x52, 0x4F, 0x30};
  unsigned char const receiveUntimedTagBytes[] = {0x30, 0x30};
  ParameterHandleValueMap receiveUntimedParameters;
  receiveUntimedParameters.emplace(
      identifier,
      VariableLengthData(receiveUntimedValueBytes, sizeof(receiveUntimedValueBytes)));
  VariableLengthData const receiveUntimedTag(
      receiveUntimedTagBytes, sizeof(receiveUntimedTagBytes));
  sender->sendInteraction(
      interactionClass, receiveUntimedParameters, receiveUntimedTag);
  evokeUntil(*receiver, [&] { return receiverReports.untimedDeliveryCount == 1U; });
  REQUIRE(receiverReports.untimedDeliveryCount == 1U);
  REQUIRE(receiverReports.untimedDeliveries.size() == 1U);
  auto const& receiveUntimed = receiverReports.untimedDeliveries.front();
  REQUIRE(receiveUntimed.interactionClass == receiverInteractionClass);
  REQUIRE(receiveUntimed.parameters.size() == 1U);
  REQUIRE(receiveUntimed.parameters.contains(identifier));
  REQUIRE(bytesOf(receiveUntimed.parameters.at(identifier)) ==
          std::vector<unsigned char>(
              receiveUntimedValueBytes,
              receiveUntimedValueBytes + sizeof(receiveUntimedValueBytes)));
  REQUIRE(bytesOf(receiveUntimed.tag) ==
          std::vector<unsigned char>(receiveUntimedTagBytes, receiveUntimedTagBytes + sizeof(receiveUntimedTagBytes)));
  REQUIRE(receiveUntimed.transport == reliable);
  REQUIRE(receiveUntimed.producer == senderHandle);
  REQUIRE_FALSE(receiveUntimed.sentRegionsSupplied);
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, TIMESTAMP));

  ParameterHandleValueMap firstParameters;
  unsigned char const firstValueBytes[] = {0x41, 0x42};
  unsigned char const firstTagBytes[] = {0x31};
  firstParameters.emplace(
      identifier,
      VariableLengthData(firstValueBytes, sizeof(firstValueBytes)));
  VariableLengthData const firstTag(firstTagBytes, sizeof(firstTagBytes));

  // Table 2: TSO preference without either regulation or a timestamp is RO.
  unsigned char const tsoUntimedValueBytes[] = {0x54, 0x53, 0x4FU};
  unsigned char const tsoUntimedTagBytes[] = {0x30};
  ParameterHandleValueMap tsoUntimedParameters;
  tsoUntimedParameters.emplace(
      identifier,
      VariableLengthData(tsoUntimedValueBytes, sizeof(tsoUntimedValueBytes)));
  VariableLengthData const tsoUntimedTag(
      tsoUntimedTagBytes, sizeof(tsoUntimedTagBytes));
  sender->sendInteraction(
      interactionClass, tsoUntimedParameters, tsoUntimedTag);
  evokeUntil(*receiver, [&] { return receiverReports.untimedDeliveryCount == 2U; });
  REQUIRE(receiverReports.untimedDeliveryCount == 2U);
  REQUIRE(receiverReports.untimedDeliveries.size() == 2U);
  auto const& tsoUntimed = receiverReports.untimedDeliveries.back();
  REQUIRE(tsoUntimed.interactionClass == receiverInteractionClass);
  REQUIRE(tsoUntimed.parameters.size() == 1U);
  REQUIRE(tsoUntimed.parameters.contains(identifier));
  REQUIRE(bytesOf(tsoUntimed.parameters.at(identifier)) ==
          std::vector<unsigned char>(
              tsoUntimedValueBytes,
              tsoUntimedValueBytes + sizeof(tsoUntimedValueBytes)));
  REQUIRE(bytesOf(tsoUntimed.tag) ==
          std::vector<unsigned char>(tsoUntimedTagBytes, tsoUntimedTagBytes + sizeof(tsoUntimedTagBytes)));
  REQUIRE(tsoUntimed.transport == reliable);
  REQUIRE(tsoUntimed.producer == senderHandle);
  REQUIRE_FALSE(tsoUntimed.sentRegionsSupplied);

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
  REQUIRE(receiverReports.untimedDeliveryCount == 2U);
  REQUIRE(receiverReports.deliveries.size() == 1U);
  REQUIRE(receiverReports.grants.empty());
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"untimed", "untimed", "receive"});
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

  // Table 2: a supplied timestamp still does not override preferred RECEIVE
  // when the sender is not time-regulating.
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, RECEIVE));
  unsigned char const receivePreferredPreRegValueBytes[] = {0x52, 0x4F, 0x30};
  unsigned char const receivePreferredPreRegTagBytes[] = {0x31, 0x30};
  ParameterHandleValueMap receivePreferredPreRegParameters;
  receivePreferredPreRegParameters.emplace(
      identifier,
      VariableLengthData(
          receivePreferredPreRegValueBytes,
          sizeof(receivePreferredPreRegValueBytes)));
  VariableLengthData const receivePreferredPreRegTag(
      receivePreferredPreRegTagBytes,
      sizeof(receivePreferredPreRegTagBytes));
  auto const receivePreferredPreRegRetraction = sender->sendInteraction(
      interactionClass,
      receivePreferredPreRegParameters,
      receivePreferredPreRegTag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE_FALSE(receivePreferredPreRegRetraction.isValid());
  evokeUntil(*receiver, [&] { return receiverReports.deliveries.size() == 2U; });
  REQUIRE(receiverReports.deliveries.size() == 2U);
  REQUIRE(receiverReports.untimedDeliveryCount == 2U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"untimed", "untimed", "receive", "receive"});
  auto const& receivePreferredPreReg = receiverReports.deliveries.back();
  REQUIRE(receivePreferredPreReg.parameters.size() == 1U);
  REQUIRE(receivePreferredPreReg.parameters.contains(identifier));
  REQUIRE(bytesOf(receivePreferredPreReg.parameters.at(identifier)) ==
          std::vector<unsigned char>(
              receivePreferredPreRegValueBytes,
              receivePreferredPreRegValueBytes + sizeof(receivePreferredPreRegValueBytes)));
  REQUIRE(bytesOf(receivePreferredPreReg.tag) ==
          std::vector<unsigned char>(receivePreferredPreRegTagBytes, receivePreferredPreRegTagBytes + sizeof(receivePreferredPreRegTagBytes)));
  REQUIRE(receivePreferredPreReg.producer == senderHandle);
  REQUIRE(receivePreferredPreReg.timeImplementation == standard_hla::mom::integer64_time);
  REQUIRE(receivePreferredPreReg.timeValue == L"6");
  REQUIRE(receivePreferredPreReg.sentOrder == RECEIVE);
  REQUIRE(receivePreferredPreReg.receivedOrder == RECEIVE);
  REQUIRE_FALSE(receivePreferredPreReg.retractionSupplied);
  REQUIRE_FALSE(receivePreferredPreReg.retractionValid);

  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, TIMESTAMP));

  // Hold preferred order and timestamp fixed; enabling regulation is the only
  // change, so this send is TSO and returns a retraction designator.
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  evokeUntil(*sender, [&] { return senderReports.timeRegulationEnabledCount == 1U; });
  REQUIRE(senderReports.timeRegulationEnabledCount == 1U);
  // Table 2: regulating with preferred TSO but no timestamp still sends RO.
  unsigned char const tsoRegulatingUntimedValueBytes[] = {0x54, 0x53, 0x31};
  unsigned char const tsoRegulatingUntimedTagBytes[] = {0x32, 0x30};
  ParameterHandleValueMap tsoRegulatingUntimedParameters;
  tsoRegulatingUntimedParameters.emplace(
      identifier,
      VariableLengthData(
          tsoRegulatingUntimedValueBytes,
          sizeof(tsoRegulatingUntimedValueBytes)));
  VariableLengthData const tsoRegulatingUntimedTag(
      tsoRegulatingUntimedTagBytes,
      sizeof(tsoRegulatingUntimedTagBytes));
  sender->sendInteraction(
      interactionClass, tsoRegulatingUntimedParameters, tsoRegulatingUntimedTag);
  evokeUntil(*receiver, [&] { return receiverReports.untimedDeliveryCount == 3U; });
  REQUIRE(receiverReports.untimedDeliveryCount == 3U);
  REQUIRE(receiverReports.untimedDeliveries.size() == 3U);
  REQUIRE(receiverReports.deliveries.size() == 2U);
  auto const& tsoRegulatingUntimed = receiverReports.untimedDeliveries.back();
  REQUIRE(tsoRegulatingUntimed.interactionClass == receiverInteractionClass);
  REQUIRE(tsoRegulatingUntimed.parameters.size() == 1U);
  REQUIRE(tsoRegulatingUntimed.parameters.contains(identifier));
  REQUIRE(bytesOf(tsoRegulatingUntimed.parameters.at(identifier)) ==
          std::vector<unsigned char>(
              tsoRegulatingUntimedValueBytes,
              tsoRegulatingUntimedValueBytes + sizeof(tsoRegulatingUntimedValueBytes)));
  REQUIRE(bytesOf(tsoRegulatingUntimed.tag) ==
          std::vector<unsigned char>(tsoRegulatingUntimedTagBytes, tsoRegulatingUntimedTagBytes + sizeof(tsoRegulatingUntimedTagBytes)));
  REQUIRE(tsoRegulatingUntimed.transport == reliable);
  REQUIRE(tsoRegulatingUntimed.producer == senderHandle);
  REQUIRE_FALSE(tsoRegulatingUntimed.sentRegionsSupplied);

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
  REQUIRE(receiverReports.deliveries.size() == 2U);
  REQUIRE(receiverReports.grants.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(sender->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  evokeUntil(*sender, [&] { return senderReports.grants.size() == 1U; });
  evokeUntil(*regulator, [&] { return regulatorReports.grants.size() == 1U; });
  REQUIRE(senderReports.grants == std::vector<std::wstring>{L"2"});
  REQUIRE(regulatorReports.grants == std::vector<std::wstring>{L"2"});
  evokeUntil(*receiver, [&] {
    return receiverReports.deliveries.size() == 3U &&
        receiverReports.grants.size() == 1U;
  });
  REQUIRE(receiverReports.untimedDeliveryCount == 3U);
  REQUIRE(receiverReports.deliveries.size() == 3U);
  REQUIRE(receiverReports.grants == std::vector<std::wstring>{L"6"});
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{
              "untimed", "untimed", "receive", "receive", "untimed", "timestamp", "grant"});
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

  evokeUntil(*sender, [&] { return senderReports.grants.size() == 1U; });
  REQUIRE(senderReports.grants == std::vector<std::wstring>{L"2"});

  // Table 2: regulating with preferred RECEIVE and no timestamp sends RO.
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, RECEIVE));
  unsigned char const receivePreferredUntimedValueBytes[] = {0x52, 0x4F, 0x31};
  unsigned char const receivePreferredUntimedTagBytes[] = {0x33, 0x30};
  ParameterHandleValueMap receivePreferredUntimedParameters;
  receivePreferredUntimedParameters.emplace(
      identifier,
      VariableLengthData(
          receivePreferredUntimedValueBytes,
          sizeof(receivePreferredUntimedValueBytes)));
  VariableLengthData const receivePreferredUntimedTag(
      receivePreferredUntimedTagBytes,
      sizeof(receivePreferredUntimedTagBytes));
  sender->sendInteraction(
      interactionClass,
      receivePreferredUntimedParameters,
      receivePreferredUntimedTag);
  evokeUntil(*receiver, [&] { return receiverReports.untimedDeliveryCount == 4U; });
  REQUIRE(receiverReports.untimedDeliveryCount == 4U);
  REQUIRE(receiverReports.untimedDeliveries.size() == 4U);
  REQUIRE(receiverReports.deliveries.size() == 3U);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{
      "untimed", "untimed", "receive", "receive", "untimed", "timestamp", "grant", "untimed"});
  auto const& receivePreferredUntimed = receiverReports.untimedDeliveries.back();
  REQUIRE(receivePreferredUntimed.interactionClass == receiverInteractionClass);
  REQUIRE(receivePreferredUntimed.parameters.size() == 1U);
  REQUIRE(receivePreferredUntimed.parameters.contains(identifier));
  REQUIRE(bytesOf(receivePreferredUntimed.parameters.at(identifier)) ==
          std::vector<unsigned char>(
              receivePreferredUntimedValueBytes,
              receivePreferredUntimedValueBytes + sizeof(receivePreferredUntimedValueBytes)));
  REQUIRE(bytesOf(receivePreferredUntimed.tag) ==
          std::vector<unsigned char>(receivePreferredUntimedTagBytes, receivePreferredUntimedTagBytes + sizeof(receivePreferredUntimedTagBytes)));
  REQUIRE(receivePreferredUntimed.transport == reliable);
  REQUIRE(receivePreferredUntimed.producer == senderHandle);
  REQUIRE_FALSE(receivePreferredUntimed.sentRegionsSupplied);

  // Table 2 also says that an explicit timestamp does not override RECEIVE
  // as the preferred order, even while this sender is time-regulating.
  unsigned char const thirdValueBytes[] = {0x52, 0x4F, 0x54};
  unsigned char const thirdTagBytes[] = {0x33};
  ParameterHandleValueMap thirdParameters;
  thirdParameters.emplace(
      identifier,
      VariableLengthData(thirdValueBytes, sizeof(thirdValueBytes)));
  VariableLengthData const thirdTag(thirdTagBytes, sizeof(thirdTagBytes));
  auto const thirdRetraction = sender->sendInteraction(
      interactionClass,
      thirdParameters,
      thirdTag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE_FALSE(thirdRetraction.isValid());
  evokeUntil(*receiver, [&] { return receiverReports.deliveries.size() == 4U; });
  REQUIRE(receiverReports.deliveries.size() == 4U);
  REQUIRE(receiverReports.untimedDeliveryCount == 4U);
  REQUIRE(receiverReports.grants == std::vector<std::wstring>{L"6"});
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{
              "untimed", "untimed", "receive", "receive", "untimed", "timestamp", "grant", "untimed", "receive"});
  auto const& third = receiverReports.deliveries.back();
  REQUIRE(third.parameters.size() == 1U);
  REQUIRE(third.parameters.contains(identifier));
  REQUIRE(bytesOf(third.parameters.at(identifier)) ==
          std::vector<unsigned char>(thirdValueBytes, thirdValueBytes + sizeof(thirdValueBytes)));
  REQUIRE(bytesOf(third.tag) ==
          std::vector<unsigned char>(thirdTagBytes, thirdTagBytes + sizeof(thirdTagBytes)));
  REQUIRE(third.producer == senderHandle);
  REQUIRE(third.timeImplementation == standard_hla::mom::integer64_time);
  REQUIRE(third.timeValue == L"8");
  REQUIRE(third.sentOrder == RECEIVE);
  REQUIRE(third.receivedOrder == RECEIVE);
  REQUIRE_FALSE(third.retractionSupplied);
  REQUIRE_FALSE(third.retractionValid);

  static_cast<void>(receiver->evokeCallback(0.1));
  REQUIRE(receiverReports.deliveries.size() == 4U);
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}
