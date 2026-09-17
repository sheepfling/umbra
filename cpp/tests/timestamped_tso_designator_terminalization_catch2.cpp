#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <memory>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped designator terminalization test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"timestamped-tso-designator-terminalization-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path testDataPath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  void receiveInteraction(
      InteractionClassHandle const&,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      rti1516_2025::RegionHandleSet const*,
      LogicalTime const&,
      rti1516_2025::OrderType,
      rti1516_2025::OrderType,
      MessageRetractionHandle const*) override {
    ++timestampedInteractionCount;
  }

  std::size_t timestampedInteractionCount = 0U;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded producer advance requests terminalize expired TSO designators",
    "[integration][development-profile][interaction-management][time-management][tso]"
    "[rti.service.send-interaction][rti.service.retract]"
    "[rti.service.time-advance-request][rti.service.time-advance-request-available]"
    "[rti.service.next-message-request][rti.service.next-message-request-available]"
    "[rti.service.flush-queue-request]") {
  ReportingFederateAmbassador reports;
  ReportingFederateAmbassador receiverReports;
  auto rti = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = testDataPath(
      "parameter-handle-provider-fom.xml").wstring();
  unsigned char const tagBytes[] = {0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"terminal-producer", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"terminal-receiver", L"subscriber", federationName));

  auto const interactionClass = rti->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = rti->getParameterHandle(
      interactionClass,
      L"Identifier");
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xE1, 0xE2};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(rti->changeInteractionOrderType(
      interactionClass,
      rti1516_2025::TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  auto sendAtBoundary = [&](std::int64_t timestamp) {
    auto const handle = rti->sendInteraction(
        interactionClass,
        parameterValues,
        tag,
        rti1516_2025::HLAinteger64Time(timestamp));
    REQUIRE(handle.isValid());
    return handle;
  };
  auto requireTerminal = [&](MessageRetractionHandle const& handle) {
    REQUIRE_THROWS_AS(
        rti->retract(handle),
        rti1516_2025::MessageCanNoLongerBeRetracted);
  };

  // With actual lookahead five, each producer advance below moves its
  // timestamped retraction boundary past the just-sent passel. The idle
  // constrained recipient retains no reflected callback while every official
  // advance form terminalizes its own designator.
  auto const tar = sendAtBoundary(6);
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  requireTerminal(tar);
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  auto const tara = sendAtBoundary(7);
  REQUIRE_NOTHROW(rti->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(2)));
  requireTerminal(tara);
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  auto const nmr = sendAtBoundary(8);
  REQUIRE_NOTHROW(rti->nextMessageRequest(
      rti1516_2025::HLAinteger64Time(3)));
  requireTerminal(nmr);
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  auto const nmra = sendAtBoundary(9);
  REQUIRE_NOTHROW(rti->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(4)));
  requireTerminal(nmra);
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  auto const fqr = sendAtBoundary(10);
  REQUIRE_NOTHROW(rti->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(5)));
  requireTerminal(fqr);
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  REQUIRE(receiverReports.timestampedInteractionCount == 0U);
  REQUIRE_NOTHROW(receiver->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(rti->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(rti->disconnect());
}
