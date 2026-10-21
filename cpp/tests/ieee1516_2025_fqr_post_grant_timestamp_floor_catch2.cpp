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
#error "The FQG post-grant timestamp-floor test requires the Umbra source directory."
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

class FqgFloorFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
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
  }

  void flushQueueGrant(LogicalTime const& time, LogicalTime const& optimisticTime)
      override {
    ++grantCount;
    grantTime = time.toString();
    optimisticGrantTime = optimisticTime.toString();
  }

  std::vector<std::wstring> receivedTimes;
  std::wstring grantTime;
  std::wstring optimisticGrantTime;
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
  return L"fqr-post-grant-timestamp-floor-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

}  // namespace

TEST_CASE(
    "Embedded FQG enforces the post-grant TSO floor at optimistic time plus lookahead",
    "[integration][development-profile][2025][time-management][tso]"
    "[fqr-post-grant-optimistic-time-lookahead-floor]"
    "[rti.service.flush-queue-request][rti.service.send-interaction]"
    "[federate.callback.flush-queue-grant]") {
  FqgFloorFederateAmbassador regulatingReports;
  FqgFloorFederateAmbassador peerReports;
  auto regulating = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("parameter-handle-provider-fom.xml").wstring();
  std::vector<unsigned char> const valueBytes{0x46U, 0x51U, 0x47U};
  std::vector<unsigned char> const tagBytes{0x46U, 0x4CU};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  REQUIRE_NOTHROW(regulating->connect(regulatingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulating->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(regulating->joinFederationExecution(
      L"fqr-floor-regulating", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"fqr-floor-peer", L"publisher", federationName));

  auto const interactionClass = regulating->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const parameter = regulating->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
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

  ParameterHandleValueMap values;
  values.emplace(parameter, VariableLengthData(valueBytes.data(), valueBytes.size()));
  auto const queued = peer->sendInteraction(
      interactionClass, values, tag, rti1516_2025::HLAinteger64Time(1));
  REQUIRE(queued.isValid());
  REQUIRE(regulatingReports.receivedTimes.empty());

  // The known early-grant setup has GALT=1, so FQR(10) completes at actual
  // and optimistic time 1. Its post-grant TSO floor is therefore 1+1=2.
  REQUIRE_NOTHROW(regulating->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  drainCallbacks(*regulating);
  REQUIRE(regulatingReports.grantCount == 1U);
  REQUIRE(regulatingReports.grantTime == L"1");
  REQUIRE(regulatingReports.optimisticGrantTime == L"1");

  REQUIRE_THROWS_AS(
      regulating->sendInteraction(
          interactionClass, values, tag, rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::InvalidLogicalTime);
  auto const atFloor = regulating->sendInteraction(
      interactionClass, values, tag, rti1516_2025::HLAinteger64Time(2));
  REQUIRE(atFloor.isValid());

  REQUIRE_NOTHROW(regulating->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulating->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulating->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
