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
#error "The strict TAR timestamp-cutoff test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::LogicalTime;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::VariableLengthData;

class TarCutoffFederateAmbassador final : public rti1516_2025::NullFederateAmbassador {
 public:
  void receiveInteraction(
      InteractionClassHandle const&,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      rti1516_2025::RegionHandleSet const*) override {
    ++receiveOrderInteractionCount;
    callbackOrder.push_back("receive-order-interaction");
  }

  void receiveInteraction(
      InteractionClassHandle const&,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      rti1516_2025::RegionHandleSet const*,
      LogicalTime const& time,
      OrderType,
      OrderType,
      rti1516_2025::MessageRetractionHandle const*) override {
    receivedTimes.push_back(time.toString());
    callbackOrder.push_back("interaction");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    grantedTimes.push_back(time.toString());
    ++grantCount;
    callbackOrder.push_back("grant");
  }

  std::vector<std::wstring> receivedTimes;
  std::vector<std::wstring> grantedTimes;
  std::vector<std::string> callbackOrder;
  std::size_t receiveOrderInteractionCount = 0U;
  std::size_t grantCount = 0U;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"tar-strict-timestamp-cutoff-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

}  // namespace

TEST_CASE(
    "Embedded TAR delivers queued RO and TSO at or before its supplied time before its grant",
    "[integration][development-profile][interaction-management][time-management]"
    "[timestamped-interaction][receive-order][tso][tso-tar-inclusive-target]"
    "[rti.service.send-interaction][rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  TarCutoffFederateAmbassador publisherReports;
  TarCutoffFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("parameter-handle-provider-fom.xml").wstring();
  std::vector<unsigned char> const valueBytes{0xD1U};
  std::vector<unsigned char> const tagBytes{0xD2U};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"tar-cutoff-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"tar-cutoff-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const parameter = publisher->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  ParameterHandleValueMap values;
  values.emplace(parameter, VariableLengthData(valueBytes.data(), valueBytes.size()));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  // A no-time Send Interaction is receive order. With asynchronous delivery
  // disabled by default, TAR must include the queued RO callback as well as
  // its eligible TSO set.
  REQUIRE_NOTHROW(publisher->sendInteraction(interactionClass, values, tag));
  REQUIRE(receiverReports.callbackOrder.empty());
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);

  // §8.8 includes queued TSO messages at the supplied time in TAR's delivery
  // set. Queue one message below the boundary, one exactly at it, and one
  // later; no future-arrival coordination is asserted.
  auto const message = publisher->sendInteraction(
      interactionClass, values, tag, rti1516_2025::HLAinteger64Time(5));
  REQUIRE(message.isValid());
  auto const equalBoundaryMessage = publisher->sendInteraction(
      interactionClass, values, tag, rti1516_2025::HLAinteger64Time(6));
  REQUIRE(equalBoundaryMessage.isValid());
  auto const laterMessage = publisher->sendInteraction(
      interactionClass, values, tag, rti1516_2025::HLAinteger64Time(11));
  REQUIRE(laterMessage.isValid());
  REQUIRE(receiverReports.receivedTimes.empty());
  // TAR(6) delivers timestamp 5 and the equal-boundary timestamp 6, but leaves
  // timestamp 11 queued. This contrasts with the adjacent FQR case, which
  // flushes a queued timestamp greater than its supplied request time.
  // Advance the producing regulator to time 1. With lookahead 5, its GALT is
  // exactly 6. TAR(6) must deliver the equal-time TSO before TAG, but it must
  // still wait after acknowledgement because TAR(6) is not strictly below
  // the regulator's unchanged GALT.
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  drainCallbacks(*publisher);
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));

  for (int pass = 0; pass != 128 && receiverReports.receivedTimes.size() < 2U; ++pass) {
    static_cast<void>(receiver->evokeCallback(0.0));
  }
  CHECK(receiverReports.receiveOrderInteractionCount == 1U);
  CHECK(receiverReports.receivedTimes ==
        std::vector<std::wstring>{L"5", L"6"});
  CHECK(receiverReports.callbackOrder ==
        std::vector<std::string>{
            "receive-order-interaction",
            "interaction",
            "interaction"});
  CHECK(receiverReports.grantCount == 0U);

  // Acknowledgement removes timestamp 6 from the incoming-delivery set, but
  // the remaining regulator still defines GALT=6. Only after it advances to
  // time 2 (GALT=7) may TAR(6) grant.
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.grantedTimes ==
          std::vector<std::wstring>{L"1", L"2"});
  for (int pass = 0; pass != 128 && receiverReports.grantCount == 0U; ++pass) {
    static_cast<void>(receiver->evokeCallback(0.0));
  }
  CHECK(receiverReports.receiveOrderInteractionCount == 1U);
  CHECK(receiverReports.receivedTimes ==
        std::vector<std::wstring>{L"5", L"6"});
  REQUIRE(receiverReports.grantCount == 1U);
  CHECK(receiverReports.grantedTimes == std::vector<std::wstring>{L"6"});
  CHECK(receiverReports.callbackOrder.size() == 4U);
  CHECK(receiverReports.callbackOrder.back() == "grant");

  // The timestamp-11 TSO remains pending beyond TAR(6). Once the producer
  // advances its watermark and the receiver requests time 12, that message is
  // delivered before the second grant.
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(8)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.grantedTimes ==
          std::vector<std::wstring>{L"1", L"2", L"8"});
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(12)));
  for (int pass = 0; pass != 128 && receiverReports.grantCount < 2U; ++pass) {
    static_cast<void>(receiver->evokeCallback(0.0));
  }
  CHECK(receiverReports.receivedTimes ==
        std::vector<std::wstring>{L"5", L"6", L"11"});
  CHECK(receiverReports.receiveOrderInteractionCount == 1U);
  CHECK(receiverReports.grantedTimes ==
        std::vector<std::wstring>{L"6", L"12"});
  CHECK(receiverReports.callbackOrder ==
        std::vector<std::string>{
            "receive-order-interaction",
            "interaction",
            "interaction",
            "grant",
            "interaction",
            "grant"});

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded TAR grants after acknowledging TSO below its target despite the acknowledged timestamp",
    "[integration][development-profile][time-management][galt][tso]"
    "[tar-acknowledged-tso-galt-release][rti.service.send-interaction]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant][2025]") {
  TarCutoffFederateAmbassador publisherReports;
  TarCutoffFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("parameter-handle-provider-fom.xml").wstring();
  std::vector<unsigned char> const valueBytes{0xD3U};
  std::vector<unsigned char> const tagBytes{0xD4U};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"tar-acknowledged-tso-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"tar-acknowledged-tso-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const parameter = publisher->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  ParameterHandleValueMap values;
  values.emplace(parameter, VariableLengthData(valueBytes.data(), valueBytes.size()));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);

  // The event at 3 is queued before the publisher advances from 0 to 5.
  // With lookahead 1, the publisher's post-grant send frontier is 6. The
  // receiver then TARs to 4, so it must first acknowledge the queued event at
  // 3 and may grant only after the TAR's delivery and safety conditions hold.
  // This case intentionally does not assert any queried GALT value or formula.
  auto const message = publisher->sendInteraction(
      interactionClass, values, tag, rti1516_2025::HLAinteger64Time(3));
  REQUIRE(message.isValid());
  REQUIRE(receiverReports.receivedTimes.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(5)));
  for (int pass = 0; pass != 128 && publisherReports.grantCount == 0U; ++pass) {
    static_cast<void>(publisher->evokeCallback(0.0));
  }
  REQUIRE(publisherReports.grantedTimes == std::vector<std::wstring>{L"5"});

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(4)));
  for (int pass = 0; pass != 128 && receiverReports.grantCount == 0U; ++pass) {
    static_cast<void>(receiver->evokeCallback(0.0));
  }
  CHECK(receiverReports.receivedTimes == std::vector<std::wstring>{L"3"});
  CHECK(receiverReports.callbackOrder ==
        std::vector<std::string>{"interaction", "grant"});
  CHECK(receiverReports.grantedTimes == std::vector<std::wstring>{L"4"});
  CHECK(receiverReports.grantCount == 1U);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
