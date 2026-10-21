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
#error "The default-region attribute test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RangeBounds;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
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
  return L"timestamped-default-region-attribute-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct TimeAdvanceGrantReport final {
    std::wstring timeImplementationName;
    std::wstring value;
  };

  struct FlushQueueGrantReport final {
    std::wstring timeImplementationName;
    std::wstring value;
    std::wstring optimisticValue;
  };

  struct ReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
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
    discoveredObjectInstances.push_back(objectInstance);
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("reflect");
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  void flushQueueGrant(
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::LogicalTime const& optimisticTime) override {
    flushQueueGrantReports.push_back({
        time.implementationName(), time.toString(), optimisticTime.toString()});
    callbackOrder.push_back("flush-grant");
  }

  std::vector<ObjectInstanceHandle> discoveredObjectInstances;
  std::vector<ReflectionReport> attributeReflectionReports;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<FlushQueueGrantReport> flushQueueGrantReports;
  std::vector<std::string> callbackOrder;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded timestamped default-region attribute update preserves 2025 regional callback metadata",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[default-region][tso][rti.service.update-attribute-values]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0xB1, 0x6E};
  unsigned char const tagBytes[] = {0x54, 0x53, 0x4F, 0x2D, 0x41};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-default-region-attribute-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-default-region-attribute-receiver",
      L"subscriber",
      federationName));

  auto const soda = publisher->getObjectClassHandle(
      fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(
      soda,
      fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(
      fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      soda,
      flavorOnly,
      TIMESTAMP));

  auto const receiverRegion = receiver->createRegion(
      rti1516_2025::DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(
      RegionHandleSet{receiverRegion}));
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  // Ordinary registration has no public source RegionHandle.  The RTI's
  // private default realization still makes the dimensional object visible to
  // the committed regional subscriber before the TSO update is accepted.
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(soda));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.discoveredObjectInstances.size() == 1U);
  REQUIRE(receiverReports.discoveredObjectInstances.front() == objectInstance);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  AttributeHandleValueMap values;
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);

  auto const& report = receiverReports.attributeReflectionReports.front();
  REQUIRE(report.objectInstance == objectInstance);
  REQUIRE(report.attributeValues.size() == 1U);
  REQUIRE(report.attributeValues.contains(flavor));
  REQUIRE(variableLengthDataBytes(report.attributeValues.at(flavor)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == TIMESTAMP);
  REQUIRE(report.receivedOrderType == TIMESTAMP);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.empty());
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().timeImplementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"6");
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"reflect", "grant"});
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded early NMR grant eases timestamped attribute update bound",
    "[integration][development-profile][2025][object-management][time-management][tso]"
    "[timestamped-attribute-update-early-grant-tso-bound-easing]"
    "[rti.service.update-attribute-values][rti.service.next-message-request]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador peerReports;
  auto requester = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const firstValueBytes[] = {0x41U, 0x54U, 0x31U};
  unsigned char const secondValueBytes[] = {0x41U, 0x54U, 0x32U};
  unsigned char const tagBytes[] = {0x45U, 0x41U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"attribute-early-grant-requester", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"attribute-early-grant-peer", L"publisher", federationName));

  auto const soda = requester->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = requester->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(peer->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(soda, flavorOnly));
  ObjectInstanceHandle const requesterObject = requester->registerObjectInstance(soda);
  ObjectInstanceHandle const peerObject = peer->registerObjectInstance(soda);
  while (requester->evokeCallback(0.0)) {
  }
  while (peer->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  while (requester->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(requester->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (requester->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(peer->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  while (peer->evokeCallback(0.0)) {
  }

  AttributeHandleValueMap peerValues;
  peerValues.emplace(flavor, VariableLengthData(firstValueBytes, sizeof(firstValueBytes)));
  auto const incomingRetraction = peer->updateAttributeValues(
      peerObject, peerValues, tag, rti1516_2025::HLAinteger64Time(5));
  REQUIRE(incomingRetraction.isValid());
  REQUIRE(requesterReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(requester->nextMessageRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(requesterReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  while (peer->evokeCallback(0.0)) {
  }
  while (requester->evokeCallback(0.0)) {
  }

  REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);
  auto const& incomingReport = requesterReports.attributeReflectionReports.front();
  REQUIRE(incomingReport.objectInstance == peerObject);
  REQUIRE(incomingReport.attributeValues.size() == 1U);
  REQUIRE(incomingReport.attributeValues.contains(flavor));
  REQUIRE(variableLengthDataBytes(incomingReport.attributeValues.at(flavor)) ==
          std::vector<unsigned char>(
              firstValueBytes, firstValueBytes + sizeof(firstValueBytes)));
  REQUIRE(incomingReport.timeValue == L"5");
  REQUIRE(incomingReport.sentOrderType == TIMESTAMP);
  REQUIRE(incomingReport.receivedOrderType == TIMESTAMP);
  REQUIRE(incomingReport.retractionSupplied);
  REQUIRE(incomingReport.retractionValid);
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.front().value == L"5");
  REQUIRE(requesterReports.callbackOrder == std::vector<std::string>{"reflect", "grant"});

  rti1516_2025::HLAinteger64Time logicalTime;
  REQUIRE_NOTHROW(requester->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);
  AttributeHandleValueMap requesterValues;
  requesterValues.emplace(flavor, VariableLengthData(secondValueBytes, sizeof(secondValueBytes)));
  auto const outgoingRetraction = requester->updateAttributeValues(
      requesterObject,
      requesterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(outgoingRetraction.isValid());
  REQUIRE_NOTHROW(requester->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);

  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(peer->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded distinct FQR grant uses actual time for timestamped attribute update bound",
    "[integration][development-profile][2025][object-management][time-management][tso]"
    "[fqr-distinct-actual-optimistic-attribute-bound]"
    "[rti.service.update-attribute-values][rti.service.flush-queue-request]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador peerReports;
  auto requester = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const incomingValueBytes[] = {0x46U, 0x51U, 0x52U, 0x41U};
  unsigned char const outgoingValueBytes[] = {0x46U, 0x51U, 0x52U, 0x42U};
  unsigned char const tagBytes[] = {0x46U, 0x51U, 0x52U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"attribute-fqr-bound-requester", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"attribute-fqr-bound-peer", L"publisher", federationName));

  auto const soda = requester->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = requester->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(peer->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(soda, flavorOnly));
  ObjectInstanceHandle const requesterObject = requester->registerObjectInstance(soda);
  ObjectInstanceHandle const peerObject = peer->registerObjectInstance(soda);
  while (requester->evokeCallback(0.0)) {
  }
  while (peer->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  while (requester->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(requester->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (requester->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(peer->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  while (peer->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(requester->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  for (int pass = 0; pass != 8; ++pass) {
    while (requester->evokeCallback(0.0)) {
    }
    while (peer->evokeCallback(0.0)) {
    }
  }
  rti1516_2025::HLAinteger64Time logicalTime;
  REQUIRE_NOTHROW(requester->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 4);
  REQUIRE_NOTHROW(peer->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 4);
  requesterReports.callbackOrder.clear();

  AttributeHandleValueMap incomingValues;
  incomingValues.emplace(
      flavor, VariableLengthData(incomingValueBytes, sizeof(incomingValueBytes)));
  auto const incomingRetraction = peer->updateAttributeValues(
      peerObject, incomingValues, tag, rti1516_2025::HLAinteger64Time(7));
  REQUIRE(incomingRetraction.isValid());
  REQUIRE_NOTHROW(requester->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  while (requester->evokeCallback(0.0)) {
  }
  REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);
  REQUIRE(requesterReports.attributeReflectionReports.front().timeValue == L"7");
  REQUIRE(requesterReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(requesterReports.flushQueueGrantReports.front().value == L"5");
  REQUIRE(requesterReports.flushQueueGrantReports.front().optimisticValue == L"7");
  REQUIRE(requesterReports.callbackOrder == std::vector<std::string>{"reflect", "flush-grant"});

  REQUIRE_NOTHROW(requester->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);
  AttributeHandleValueMap outgoingValues;
  outgoingValues.emplace(
      flavor, VariableLengthData(outgoingValueBytes, sizeof(outgoingValueBytes)));
  auto const outgoingRetraction = requester->updateAttributeValues(
      requesterObject,
      outgoingValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(outgoingRetraction.isValid());
  REQUIRE_NOTHROW(requester->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);

  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
