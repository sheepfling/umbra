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
#error "The strict TARA timestamp-cutoff test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::LogicalTime;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RECEIVE;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::VariableLengthData;

class CutoffFederateAmbassador final : public rti1516_2025::NullFederateAmbassador {
 public:
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
  return L"tara-strict-timestamp-cutoff-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

}  // namespace

TEST_CASE(
    "Embedded TARA delivers a queued TSO strictly earlier than its supplied time before its grant",
    "[integration][development-profile][interaction-management][time-management]"
    "[timestamped-interaction][tso][tso-tara-strict-less-than-cutoff]"
    "[rti.service.send-interaction][rti.service.time-advance-request-available]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  CutoffFederateAmbassador publisherReports;
  CutoffFederateAmbassador receiverReports;
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
      L"tara-cutoff-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"tara-cutoff-receiver", L"subscriber", federationName));

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
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);

  // This message is queued before TARA(6); its timestamp is strictly less
  // than the supplied time. The test makes no claim about later arrivals.
  auto const message = publisher->sendInteraction(
      interactionClass, values, tag, rti1516_2025::HLAinteger64Time(5));
  REQUIRE(message.isValid());
  REQUIRE(receiverReports.receivedTimes.empty());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);

  for (int pass = 0; pass != 128 && receiverReports.grantCount == 0U; ++pass) {
    static_cast<void>(receiver->evokeCallback(0.0));
  }
  REQUIRE(receiverReports.grantCount == 1U);
  REQUIRE(receiverReports.receivedTimes == std::vector<std::wstring>{L"5"});
  REQUIRE(receiverReports.callbackOrder.size() >= 2U);
  REQUIRE(receiverReports.callbackOrder.front() == "interaction");
  REQUIRE(receiverReports.callbackOrder[1] == "grant");

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded TARA delivers an earlier TSO that arrives after its request before grant",
    "[integration][development-profile][time-management][galt][tso]"
    "[tso-tara-queued-pregrant-arrival][rti.service.time-advance-request-available]"
    "[rti.service.time-advance-request][rti.service.send-interaction]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]"
    "[2025]") {
  CutoffFederateAmbassador publisherReports;
  CutoffFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("parameter-handle-provider-fom.xml").wstring();
  std::vector<unsigned char> const valueBytes{0xD5U};
  std::vector<unsigned char> const tagBytes{0xD6U};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"tara-late-queued-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"tara-late-queued-receiver", L"subscriber", federationName));

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

  // Move the producer to 4 (send frontier 5), start TARA(6), then enqueue
  // timestamp 5 before the producer advances to 5 (send frontier 6). The
  // already-queued pre-grant delivery must not pin GALT below TARA(6).
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.grantedTimes == std::vector<std::wstring>{L"4"});
  REQUIRE_NOTHROW(receiver->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(6)));
  auto const message = publisher->sendInteraction(
      interactionClass, values, tag, rti1516_2025::HLAinteger64Time(5));
  REQUIRE(message.isValid());
  REQUIRE(receiverReports.receivedTimes.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(5)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.grantedTimes ==
          std::vector<std::wstring>{L"4", L"5"});

  for (int pass = 0; pass != 128 && receiverReports.grantCount == 0U; ++pass) {
    static_cast<void>(receiver->evokeCallback(0.0));
  }
  CHECK(receiverReports.receivedTimes == std::vector<std::wstring>{L"5"});
  CHECK(receiverReports.callbackOrder ==
        std::vector<std::string>{"interaction", "grant"});
  CHECK(receiverReports.grantedTimes == std::vector<std::wstring>{L"6"});
  REQUIRE(receiverReports.grantCount == 1U);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
