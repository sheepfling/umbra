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
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped attribute-update no-fanout test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"timestamped-attribute-update-no-fanout-" +
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
    "Embedded timestamped Update Attribute Values returns a retraction designator without recipient fanout",
    "[integration][development-profile][object-management][time-management][tso]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.time-advance-request]") {
  rti1516_2025::NullFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  unsigned char const valueBytes[] = {0x4EU, 0x46U};
  unsigned char const tagBytes[] = {0x4EU, 0x4FU, 0x2DU, 0x41U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"attribute-no-fanout-producer",
      L"publisher",
      federationName));

  auto const objectClass = rti->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const attribute = rti->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::reliable_base_a);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(rti->changeDefaultAttributeOrderType(
      objectClass,
      attributes,
      TIMESTAMP));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = rti->registerObjectInstance(objectClass));
  REQUIRE_NOTHROW(rti->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  AttributeHandleValueMap values;
  values.emplace(
      attribute,
      VariableLengthData(valueBytes, sizeof(valueBytes)));

  // Clause 6.10's TSO-preferred-attribute condition is independent of
  // recipient fanout. The public result therefore carries a valid designator
  // even though this one-federate execution has no subscriber.
  auto const retractable = rti->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retractable.isValid());
  REQUIRE_NOTHROW(rti->retract(retractable));
  REQUIRE_THROWS_AS(
      rti->retract(retractable),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  auto const expired = rti->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(expired.isValid());
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_THROWS_AS(
      rti->retract(expired),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  REQUIRE_NOTHROW(rti->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
