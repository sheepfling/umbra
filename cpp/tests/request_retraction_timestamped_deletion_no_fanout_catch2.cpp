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
#error "The timestamped deletion retraction test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"request-retraction-timestamped-deletion-no-fanout-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path sourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct RequestRetractionReport final {
    bool retractionValid = false;
  };

  void requestRetraction(
      rti1516_2025::MessageRetractionHandle const& retraction) override {
    requestRetractionReports.push_back({retraction.isValid()});
  }

  std::vector<RequestRetractionReport> requestRetractionReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded Request Retraction reconstitutes timestamped deletion without recipient fanout",
    "[integration][development-profile][object-management][time-management]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  auto publisher = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0xD6, 0x48, 0x3C};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-delete-no-fanout-publisher", L"publisher", federationName));

  auto const child = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  AttributeHandleSet const attributes{reliable};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  // Equality at current time plus actual lookahead is not retractable under
  // 8.22.3's strict condition, even though the outgoing deletion itself is a
  // valid timestamped message.
  ObjectInstanceHandle boundaryObject;
  REQUIRE_NOTHROW(boundaryObject = publisher->registerObjectInstance(child));
  auto const boundaryRetraction = publisher->deleteObjectInstance(
      boundaryObject,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(boundaryRetraction.isValid());
  REQUIRE_THROWS_AS(
      publisher->retract(boundaryRetraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  // With no other recipient, no typed payload enters the temporal queue. The
  // execution still owns the designator and invocation snapshot, so a legal
  // later Retract must reconstitute the object without emitting a callback.
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);
  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE_THROWS_AS(
      publisher->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE(publisher->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, reliable));
  REQUIRE(publisherReports.requestRetractionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
}
