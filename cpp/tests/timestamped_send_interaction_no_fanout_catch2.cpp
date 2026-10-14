#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped Send Interaction no-fanout test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"timestamped-send-interaction-no-fanout-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded timestamped Send Interaction returns a retraction designator without recipient fanout",
    "[integration][development-profile][interaction-management][time-management][tso]"
    "[rti.service.send-interaction][rti.service.retract]"
    "[rti.service.time-advance-request]") {
  rti1516_2025::NullFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath(
      "parameter-handle-provider-fom.xml").wstring();
  std::vector<unsigned char> const tagBytes{
      0x4EU, 0x4FU, 0x4EU, 0x45U};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"no-fanout-producer",
      L"publisher",
      federationName));

  auto const interactionClass = rti->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = rti->getParameterHandle(
      interactionClass,
      L"Identifier");
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  std::vector<unsigned char> const identifierBytes{0xA4U, 0xA5U};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes.data(), identifierBytes.size()));

  REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(rti->changeInteractionOrderType(
      interactionClass,
      rti1516_2025::TIMESTAMP));
  REQUIRE_NOTHROW(rti->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  // Clause 8.22.3 requires a designator even when no recipient is eligible;
  // the retraction ledger remains authoritative without retaining fanout data.
  auto const retractable = rti->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retractable.isValid());
  REQUIRE_NOTHROW(rti->retract(retractable));
  REQUIRE_THROWS_AS(
      rti->retract(retractable),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  auto const expired = rti->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(expired.isValid());
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_THROWS_AS(
      rti->retract(expired),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  REQUIRE_NOTHROW(rti->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

namespace {

class TimestampedInteractionRecorder final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const&,
      rti1516_2025::ParameterHandleValueMap const&,
      rti1516_2025::VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      rti1516_2025::RegionHandleSet const*,
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::OrderType,
      rti1516_2025::OrderType,
      rti1516_2025::MessageRetractionHandle const*) override {
    ++receivedCount;
    timestampImplementation = time.implementationName();
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      timestampValue = integerTime->getTime();
    }
  }

  void timeConstrainedEnabled(rti1516_2025::LogicalTime const&) override {
    ++timeConstrainedEnabledCount;
  }

  void timeRegulationEnabled(rti1516_2025::LogicalTime const&) override {
    ++timeRegulationEnabledCount;
  }

  std::size_t receivedCount = 0U;
  std::size_t timeConstrainedEnabledCount = 0U;
  std::size_t timeRegulationEnabledCount = 0U;
  std::wstring timestampImplementation;
  std::int64_t timestampValue = -1;
};

}  // namespace

TEST_CASE(
    "A non-regulating federate sends a timestamped interaction when GALT is defined",
    "[integration][development-profile][time-management][timestamped-interaction]"
    "[unregulated-federate-timestamped-interaction]"
    "[rti.service.send-interaction][rti.service.query-galt]"
    "[rti.service.query-logical-time]"
    "[rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.time-constrained-enabled]"
    "[federate.callback.time-regulation-enabled][2025]") {
  TimestampedInteractionRecorder senderAmbassador;
  TimestampedInteractionRecorder receiverAmbassador;
  TimestampedInteractionRecorder regulatorAmbassador;
  auto sender = makeRti();
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath(
      "parameter-handle-provider-fom.xml").wstring();
  std::vector<unsigned char> const tagBytes{0x55U, 0x4EU, 0x52U};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  REQUIRE_NOTHROW(sender->connect(senderAmbassador, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverAmbassador, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->connect(regulatorAmbassador, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(sender->joinFederationExecution(
      L"unregulated-timestamp-producer",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"unregulated-timestamp-receiver",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(
      L"timestamp-regulator",
      L"regulator",
      federationName));

  auto const interactionClass = sender->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  auto const identifier = sender->getParameterHandle(
      interactionClass,
      L"Identifier");
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  auto const receiverInteractionClass = receiver->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild");
  REQUIRE(receiverInteractionClass.isValid());

  ParameterHandleValueMap parameterValues;
  std::vector<unsigned char> const identifierBytes{0xD1U, 0xD2U};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes.data(), identifierBytes.size()));

  REQUIRE_NOTHROW(sender->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(
      interactionClass,
      rti1516_2025::TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(receiverInteractionClass));

  // §4.19 permits this non-regulating sender's timestamp because its GALT is
  // defined and timestamp 6 exceeds the constrained receiver's position.
  // Section 8 limits TSO receipt to time-constrained federates.
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  for (int attempt = 0;
       attempt < 4 && regulatorAmbassador.timeRegulationEnabledCount == 0U;
       ++attempt) {
    static_cast<void>(regulator->evokeCallback(0.05));
  }
  REQUIRE(regulatorAmbassador.timeRegulationEnabledCount == 1U);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  for (int attempt = 0;
       attempt < 4 && receiverAmbassador.timeConstrainedEnabledCount == 0U;
       ++attempt) {
    static_cast<void>(receiver->evokeCallback(0.05));
  }
  REQUIRE(receiverAmbassador.timeConstrainedEnabledCount == 1U);

  rti1516_2025::HLAinteger64Time senderGalt;
  REQUIRE(sender->queryGALT(senderGalt));
  REQUIRE(senderGalt.getTime() == 5);
  rti1516_2025::HLAinteger64Time receiverPosition;
  REQUIRE_NOTHROW(receiver->queryLogicalTime(receiverPosition));
  REQUIRE(receiverPosition.getTime() == 0);

  REQUIRE_NOTHROW(sender->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  for (int attempt = 0;
       attempt < 4 && receiverAmbassador.receivedCount == 0U;
       ++attempt) {
    static_cast<void>(regulator->evokeCallback(0.05));
    static_cast<void>(receiver->evokeCallback(0.05));
  }
  REQUIRE(receiverAmbassador.receivedCount == 1U);
  REQUIRE(receiverAmbassador.timestampImplementation == L"HLAinteger64Time");
  REQUIRE(receiverAmbassador.timestampValue == 6);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}
