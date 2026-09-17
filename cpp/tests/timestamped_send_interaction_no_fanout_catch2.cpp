#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <memory>
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
