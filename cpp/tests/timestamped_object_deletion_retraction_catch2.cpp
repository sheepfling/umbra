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
#error "The timestamped object-deletion retraction test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::LogicalTime;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::VariableLengthData;

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"timestamped-object-deletion-retraction-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path sourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct RemovalReport final {
    ObjectInstanceHandle objectInstance;
    VariableLengthData tag;
    FederateHandle producer;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct TimeReport final {
    std::wstring implementationName;
    std::wstring value;
  };

  struct RequestRetractionReport final {
    bool retractionValid = false;
    VariableLengthData encodedRetraction;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& tag,
      FederateHandle const& producer,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    objectRemovalReports.push_back({
        objectInstance,
        tag,
        producer,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("remove");
  }

  void requestRetraction(
      rti1516_2025::MessageRetractionHandle const& retraction) override {
    requestRetractionReports.push_back({retraction.isValid(), retraction.encode()});
    callbackOrder.push_back("request-retraction");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<RemovalReport> objectRemovalReports;
  std::vector<RequestRetractionReport> requestRetractionReports;
  std::vector<TimeReport> timeAdvanceGrantReports;
  std::vector<std::string> callbackOrder;
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
    "Embedded Request Retraction reconstitutes a delivered timestamped object deletion",
    "[integration][development-profile][object-management][time-management][tso]"
    "[retraction][multi-federate-callback-ordering]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request]"
    "[federate.callback.remove-object-instance][federate.callback.request-retraction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador constrainedReports;
  auto publisher = makeRti();
  auto immediate = makeRti();
  auto constrained = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "attribute-update-passel-fom.xml").wstring();
  unsigned char const tagBytes[] = {0x52U, 0x45U, 0x54U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"object-retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"object-retraction-immediate", L"subscriber", federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"object-retraction-constrained", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::best_effort_base);
  REQUIRE(objectClass.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(constrained->subscribeObjectClassAttributes(objectClass, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
  drainCallbacks(*immediate);
  drainCallbacks(*constrained);
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(constrainedReports.objectDiscoveryReports.size() == 1U);
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);

  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  drainCallbacks(*constrained);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);

  // The nonconstrained recipient receives the timestamped deletion as soon as
  // its evoked callback runs; the constrained recipient remains pending.
  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(retraction.isValid());
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.objectRemovalReports.size() == 1U);
  REQUIRE(constrainedReports.objectRemovalReports.empty());
  auto const& removal = immediateReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.producer == publisherHandle);
  REQUIRE(variableLengthDataBytes(removal.tag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(removal.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(removal.timeValue == L"2");
  REQUIRE(removal.sentOrderType == TIMESTAMP);
  REQUIRE(removal.receivedOrderType == RECEIVE);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);

  // Retract restores the invocation-time object/name/ownership snapshot before
  // queuing Request Retraction for the recipient whose callback was delivered.
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 1U);
  REQUIRE(immediateReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(immediateReports.callbackOrder ==
          std::vector<std::string>{"remove", "request-retraction"});
  REQUIRE(constrainedReports.requestRetractionReports.empty());
  REQUIRE(publisher->getObjectInstanceName(objectInstance) == objectInstanceName);
  REQUIRE(immediate->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(constrained->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, reliable));
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, bestEffort));
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  // The still-pending constrained fanout was suppressed by the same retract;
  // its advance reaches the grant boundary without a stale removal callback.
  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  drainCallbacks(*constrained);
  REQUIRE(constrainedReports.objectRemovalReports.empty());
  REQUIRE(constrainedReports.requestRetractionReports.empty());
  REQUIRE(constrainedReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(constrainedReports.callbackOrder == std::vector<std::string>{"grant"});

  REQUIRE_NOTHROW(constrained->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
