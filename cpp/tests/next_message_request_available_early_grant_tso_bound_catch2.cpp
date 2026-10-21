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

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The queued NMRA timestamp-bound test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

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

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"nmra-early-grant-tso-bound-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

class Report final : public rti1516_2025::NullFederateAmbassador {
 public:
  void receiveInteraction(
      InteractionClassHandle const&,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      rti1516_2025::RegionHandleSet const*,
      LogicalTime const&,
      OrderType,
      OrderType,
      MessageRetractionHandle const*) override {
    ++receivedInteractions;
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    ++grants;
    grantTime = time.toString();
  }

  int receivedInteractions = 0;
  int grants = 0;
  std::wstring grantTime;
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
    "Embedded NMRA grant below request eases the subsequent TSO timestamp bound",
    "[integration][development-profile][interaction-management][time-management]"
    "[nmra-early-grant-tso-bound-easing][2025][rti.service.next-message-request-available]"
    "[rti.service.send-interaction][rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  Report regulatingReports;
  Report peerReports;
  auto regulating = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" /
                          "tests" / "data" / "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const payload[] = {0x4EU, 0x4DU, 0x52U, 0x41U};
  unsigned char const tagBytes[] = {0x45U, 0x41U, 0x52U, 0x4CU, 0x59U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(regulating->connect(regulatingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulating->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(regulating->joinFederationExecution(
      L"nmra-early-grant-regulating", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"nmra-early-grant-peer", L"publisher", federationName));

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

  ParameterHandleValueMap incomingValues;
  incomingValues.emplace(identifier, VariableLengthData(payload, sizeof(payload)));
  auto const incoming = peer->sendInteraction(
      interactionClass, incomingValues, tag, rti1516_2025::HLAinteger64Time(5));
  REQUIRE(incoming.isValid());

  // The queued timestamp-5 message lets NMRA(10) grant at 5. Once the
  // federate leaves Time Advancing, the legal lower bound is current time 5
  // plus lookahead 1, rather than the stale request 10 plus lookahead 1.
  REQUIRE_NOTHROW(regulating->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(regulatingReports.grants == 0);
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*peer);
  drainCallbacks(*regulating);
  REQUIRE(regulatingReports.receivedInteractions == 1);
  REQUIRE(regulatingReports.grants == 1);
  REQUIRE(regulatingReports.grantTime == L"5");

  rti1516_2025::HLAinteger64Time logicalTime;
  REQUIRE_NOTHROW(regulating->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);

  ParameterHandleValueMap outgoingValues;
  outgoingValues.emplace(identifier, VariableLengthData(payload, sizeof(payload)));
  auto const outgoing = regulating->sendInteraction(
      interactionClass, outgoingValues, tag, rti1516_2025::HLAinteger64Time(6));
  REQUIRE(outgoing.isValid());
  REQUIRE_NOTHROW(regulating->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);

  REQUIRE_NOTHROW(regulating->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(peer->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(regulating->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulating->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
