#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The local-delete timestamped object-removal test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

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
  return L"local-delete-timestamped-object-removal-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ObjectRemovalReport final {
    ObjectInstanceHandle objectInstance;
    VariableLengthData userSuppliedTag;
    FederateHandle producingFederate;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
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
      VariableLengthData const& userSuppliedTag,
      FederateHandle const& producingFederate,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    objectRemovalReports.push_back({
        objectInstance,
        userSuppliedTag,
        producingFederate,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("remove");
  }

  void timeConstrainedEnabled(LogicalTime const&) override {
    timeConstrainedEnabledReports.push_back({});
  }

  void timeRegulationEnabled(LogicalTime const&) override {
    timeRegulationEnabledReports.push_back({});
  }

  void timeAdvanceGrant(LogicalTime const&) override {
    timeAdvanceGrantReports.push_back({});
    callbackOrder.push_back("grant");
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<ObjectRemovalReport> objectRemovalReports;
  std::vector<int> timeConstrainedEnabledReports;
  std::vector<int> timeRegulationEnabledReports;
  std::vector<int> timeAdvanceGrantReports;
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
    "Embedded local deletion isolates queued timestamped object removals per recipient",
    "[integration][development-profile][federation-management][object-management]"
    "[time-management][timestamped-object-deletion][local-delete-object-instance]"
    "[multi-federate-callback-ordering][tso]"
    "[rti.service.delete-object-instance][rti.service.local-delete-object-instance]"
    "[rti.service.time-advance-request][federate.callback.remove-object-instance]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador localReports;
  ReportingFederateAmbassador survivingReports;
  auto publisher = makeRti();
  auto local = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  std::vector<unsigned char> const tagBytes{
      0x4CU, 0x4FU, 0x43U, 0x41U, 0x4CU, 0x44U, 0x45U, 0x4CU};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(local->connect(localReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"local-delete-timestamped-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(local->joinFederationExecution(
      L"local-delete-timestamped-local-receiver",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"local-delete-timestamped-surviving-receiver",
      L"subscriber",
      federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = publisher->getAttributeHandle(child, L"ReliableBaseA");
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  AttributeHandleSet const attributes{reliable};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(local->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      child,
      attributes,
      TIMESTAMP));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE(objectInstance.isValid());
  auto const objectName = publisher->getObjectInstanceName(objectInstance);
  drainCallbacks(*local);
  drainCallbacks(*surviving);
  REQUIRE(localReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(local->enableTimeConstrained());
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  drainCallbacks(*local);
  drainCallbacks(*surviving);
  REQUIRE(localReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(survivingReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE_NOTHROW(local->localDeleteObjectInstance(objectInstance));
  REQUIRE_THROWS_AS(
      local->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_NOTHROW(local->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  drainCallbacks(*local);
  drainCallbacks(*surviving);

  // Local deletion suppresses only the invoking recipient's queued removal;
  // the independent survivor still receives the original TSO callback.
  REQUIRE(localReports.objectRemovalReports.empty());
  REQUIRE(localReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(localReports.callbackOrder == std::vector<std::string>{"grant"});
  REQUIRE(survivingReports.objectRemovalReports.size() == 1U);
  REQUIRE(survivingReports.timeAdvanceGrantReports.size() == 1U);
  auto const removalPosition = std::find(
      survivingReports.callbackOrder.begin(),
      survivingReports.callbackOrder.end(),
      "remove");
  auto const grantPosition = std::find(
      survivingReports.callbackOrder.begin(),
      survivingReports.callbackOrder.end(),
      "grant");
  REQUIRE(removalPosition != survivingReports.callbackOrder.end());
  REQUIRE(grantPosition != survivingReports.callbackOrder.end());
  REQUIRE(removalPosition < grantPosition);
  auto const& removal = survivingReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(variableLengthDataBytes(removal.userSuppliedTag) == tagBytes);
  REQUIRE(removal.producingFederate == publisherHandle);
  REQUIRE(removal.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(removal.timeValue == L"6");
  REQUIRE(removal.sentOrderType == TIMESTAMP);
  REQUIRE(removal.receivedOrderType == TIMESTAMP);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);
  REQUIRE(survivingReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(objectName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(local->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(surviving->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(local->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
