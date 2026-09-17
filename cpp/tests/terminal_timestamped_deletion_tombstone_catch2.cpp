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

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The terminal timestamped deletion test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"terminal-timestamped-deletion-tombstone-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path sourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / relativePath;
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded terminal timestamped deletion tombstone releases its object name",
    "[integration][development-profile][object-management][time-management][tso]"
    "[timestamped-deletion-tombstone]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[rti.service.time-advance-request][rti.service.reserve-object-instance-name]") {
  rti1516_2025::NullFederateAmbassador ownerReports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x54, 0x4F, 0x4D};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const objectName = L"Umbra.TerminalTimestampedDeletion";

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"terminal-deletion-owner", L"owner", federationName));

  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::best_effort_base);
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(objectName));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  ObjectInstanceHandle original;
  REQUIRE_NOTHROW(original = owner->registerObjectInstance(child, objectName));
  REQUIRE(original.isValid());
  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  auto const deletion = owner->deleteObjectInstance(
      original,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(deletion.isValid());

  // The request boundary is 1 + the actual lookahead 5, exactly the sent
  // timestamp. Clause 8.22.3's strict comparison makes the designator
  // terminal, allowing the retained deletion state and name to be released.
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE_THROWS_AS(
      owner->retract(deletion),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(objectName));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  ObjectInstanceHandle replacement;
  REQUIRE_NOTHROW(replacement = owner->registerObjectInstance(child, objectName));
  REQUIRE(replacement.isValid());

  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}
