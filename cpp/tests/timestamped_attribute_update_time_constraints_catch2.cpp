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
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped attribute-update time-constraint test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::LogicalTime;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

class GrantTrackingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  void timeAdvanceGrant(LogicalTime const& time) override {
    grants.push_back(time.toString());
  }

  std::vector<std::wstring> grants;
};

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"timestamped-attribute-update-time-constraints-" +
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
    "Timestamped Update Attribute Values rejects a timestamp below current time plus lookahead",
    "[integration][development-profile][2025][object-management][time-management]"
    "[timestamped-attribute-update][tso][timestamped-attribute-update-time-constraints]"
    "[rti.service.update-attribute-values]") {
  rti1516_2025::NullFederateAmbassador federateAmbassador;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  unsigned char const attributeBytes[] = {0x41U};
  unsigned char const tagBytes[] = {0x54U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(rti->connect(federateAmbassador, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"timestamp-boundary-publisher", L"publisher", federationName));

  auto const objectClass = rti->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const attribute = rti->getAttributeHandle(
      objectClass, fixture_hla::fixture::reliable_base_a);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  AttributeHandleValueMap values;
  values.emplace(attribute, VariableLengthData(attributeBytes, sizeof(attributeBytes)));
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(rti->changeDefaultAttributeOrderType(
      objectClass, attributes, rti1516_2025::TIMESTAMP));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = rti->registerObjectInstance(objectClass));

  REQUIRE_NOTHROW(rti->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (rti->evokeCallback(0.0)) {
  }

  // Clause 8.1.5 constrains a time-regulating sender's timestamp by current
  // logical time plus lookahead. At current time 0 and lookahead 5, timestamp
  // 4 is invalid; timestamp 6 is the positive control above that lower bound.
  REQUIRE_THROWS_AS(
      rti->updateAttributeValues(
          objectInstance,
          values,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE_NOTHROW(rti->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6)));

  REQUIRE_NOTHROW(rti->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Timestamped Update Attribute Values uses requested time plus lookahead while advancing",
    "[integration][development-profile][2025][object-management][time-management]"
    "[timestamped-attribute-update][tso][time-advancing]"
    "[timestamped-attribute-update-time-constraints]"
    "[rti.service.update-attribute-values][rti.service.time-advance-request]") {
  GrantTrackingFederateAmbassador senderReports;
  GrantTrackingFederateAmbassador peerReports;
  auto sender = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  unsigned char const attributeBytes[] = {0x42U};
  unsigned char const tagBytes[] = {0x55U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(sender->joinFederationExecution(
      L"timestamp-advancing-sender", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"timestamp-advancing-peer", L"publisher", federationName));

  auto const objectClass = sender->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const attribute = sender->getAttributeHandle(
      objectClass, fixture_hla::fixture::reliable_base_a);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  AttributeHandleValueMap values;
  values.emplace(attribute, VariableLengthData(attributeBytes, sizeof(attributeBytes)));
  REQUIRE_NOTHROW(sender->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(sender->changeDefaultAttributeOrderType(
      objectClass, attributes, rti1516_2025::TIMESTAMP));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = sender->registerObjectInstance(objectClass));

  REQUIRE_NOTHROW(sender->enableTimeConstrained());
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_NOTHROW(peer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (sender->evokeCallback(0.0)) {
  }
  while (peer->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(sender->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(10)));
  while (sender->evokeCallback(0.0)) {
  }
  REQUIRE(senderReports.grants.empty());

  // Table 1 and §8.1.5 use requested time plus lookahead during a TAR. With
  // tR=10 and lookahead 5, ts < 15 is forbidden; equality and later times are
  // allowed. The peer keeps TAR(10) pending while this check is made.
  REQUIRE_THROWS_AS(
      sender->updateAttributeValues(
          objectInstance,
          values,
          tag,
          rti1516_2025::HLAinteger64Time(14)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE_NOTHROW(sender->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(15)));
  REQUIRE_NOTHROW(sender->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(16)));

  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(10)));
  while (sender->evokeCallback(0.0)) {
  }
  while (peer->evokeCallback(0.0)) {
  }
  REQUIRE(senderReports.grants == std::vector<std::wstring>{L"10"});
  REQUIRE(peerReports.grants == std::vector<std::wstring>{L"10"});

  REQUIRE_NOTHROW(sender->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(sender->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
